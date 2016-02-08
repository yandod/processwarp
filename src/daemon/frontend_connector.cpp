
#include <uv.h>

#include <cassert>
#include <string>
#include <vector>

#include "convert.hpp"
#include "daemon_define.hpp"
#include "frontend_connector.hpp"
#include "router.hpp"
#include "worker_connector.hpp"

namespace processwarp {
/**
 * FrontendConnector instance getter as singleton pattern.
 * @return The singleton instance of FrontendConnector class.
 */
FrontendConnector& FrontendConnector::get_instance() {
  static FrontendConnector instance;
  return instance;
}

/**
 * Constructor for singleton pattern.
 * Set default value to instance members.
 * This class is singleton.
 * This method is private.
 */
FrontendConnector::FrontendConnector() :
    gui_pipe(nullptr) {
}

/**
 * Initialize for FrontendConnector.
 * Create UNIX domain socket for controller.
 * @param loop libuv's loop for WorkerConnector.
 * @param pipe_path Path of pipe that for connecting with worker.
 */
void FrontendConnector::initialize(uv_loop_t* loop, const std::string& pipe_path) {
  Connector::initialize(loop, pipe_path);
}


/**
 * Send create command to the GUi type frontend.
 * @param pid Target process-id.
 */
void FrontendConnector::create_gui(const vpid_t& pid) {
  assert(pid != SpecialPID::BROADCAST);

  if (gui_pipe == nullptr) {
    /// @todo error
    assert(false);
  }

  picojson::object data;
  data.insert(std::make_pair("command", picojson::value(std::string("create"))));
  data.insert(std::make_pair("pid", Convert::vpid2json(pid)));

  send_data(*gui_pipe, data);
}

/**
 * Relay command to frontend.
 * Packet format: {<br/>
 *   command: 'relay_command',
 *   pid: &lt;Process-id of packet&gt;
 *   dst_nid: &lt;Destination node-id&gt;
 *   src_nid: &lt;Source node-id&gt;
 *   content: &lt;Command content&gt;
 * }
 * @param packet Command packet for relaying.
 */
void FrontendConnector::relay_frontend_command(const CommandPacket& packet) {
  assert(packet.module == Module::CONTROLLER || packet.module == Module::GUI);

  if (gui_pipe == nullptr) return;

  picojson::object data;
  data.insert(std::make_pair("command", picojson::value(std::string("relay_command"))));
  data.insert(std::make_pair("pid", Convert::vpid2json(packet.pid)));
  data.insert(std::make_pair("dst_nid", Convert::nid2json(packet.dst_nid)));
  data.insert(std::make_pair("src_nid", Convert::nid2json(packet.src_nid)));
  data.insert(std::make_pair("module", Convert::int2json(packet.module)));
  data.insert(std::make_pair("content", picojson::value(packet.content)));

  send_data(*gui_pipe, data);
}

/**
 * Create pipe for controller when accept connecting.
 * Set status to SETUP.
 * @param client Pipe connect with controller by libuv.
 */
void FrontendConnector::on_connect(uv_pipe_t& client) {
  FrontendProperty property;

  property.status = PipeStatus::SETUP;
  properties.insert(std::make_pair(&client, property));
}

/**
 * When receive data, call capable methods.
 * @param client Frontend pipe that send this packet.
 * @param data Packet.
 */
void FrontendConnector::on_recv_data(uv_pipe_t& client, picojson::object& data) {
  const std::string& command = data.at("command").get<std::string>();

  if (command == "relay_command") {
    recv_relay_command(client, data);

  } else if (command == "connect_frontend") {
    recv_connect_frontend(client, data);

  } else if (command == "open_file") {
    recv_open_file(client, data);

  } else {
    /// @todo error
    assert(false);
  }
}

/**
 * When close pipe, remove status from properties.
 * If client type is GUI, set GUI pipe null.
 * @param client Target pipe to close.
 */
void FrontendConnector::on_close(uv_pipe_t& client) {
  properties.erase(&client);
  if (gui_pipe == &client) {
    gui_pipe = nullptr;
  }
}

/**
 * When receive connect-frontend command from frontend, check account.
 * Reply code 0 if account is valid, and change status to CONNECT.
 * Reply code -1 if account is invalid.
 * If client type is GUI, set GUI pipe to this pipe.
 * @param client Frontend that passed this request.
 * @param param Parameter contain account, password, frontend type.
 */
void FrontendConnector::recv_connect_frontend(uv_pipe_t& client, picojson::object& param) {
  Router& router = Router::get_instance();
  const std::string& type = param.at("type").get<std::string>();
  FrontendProperty& property = properties.at(&client);

  assert(property.status == PipeStatus::SETUP);

  if (type == "gui") {
    property.type = FrontendType::GUI;

  } else if (type == "cui") {
    property.type = FrontendType::CUI;

  } else {
    /// @todo error
    assert(false);
  }

  if (router.check_account(param.at("account").get<std::string>(),
                           param.at("password").get<std::string>())) {
    if (property.type == FrontendType::GUI) {
      if (gui_pipe == nullptr) {
        gui_pipe = &client;

      } else {
        send_connect_frontend(client, -1, SpecialNID::NONE);
        close(client);
        return;
      }
    }
    send_connect_frontend(client, 0, router.get_my_nid());
    property.status = PipeStatus::CONNECT;

  } else {
    send_connect_frontend(client, -1, SpecialNID::NONE);
    close(client);
  }
}

/**
 * When receive open-file command from frontend, pass capable method on Router.
 * @param client Frontend that passed this request.
 * @param param Parameter contain a filename to open.
 */
void FrontendConnector::recv_open_file(uv_pipe_t& client, picojson::object& param) {
  Router& router = Router::get_instance();

  router.load_llvm(param.at("filename").get<std::string>(), std::vector<std::string>());
}

/**
 * When receive command from frontend, relay to capable module.
 * @param client Frontend that passed this request.
 * @param content Data contain target module, pid, content of command.
 */
void FrontendConnector::recv_relay_command(uv_pipe_t& client, picojson::object& content) {
  CommandPacket packet = {
    Convert::json2vpid(content.at("pid")),
    Convert::json2nid(content.at("dst_nid")),
    SpecialNID::NONE,
    Convert::json2int<Module::Type>(content.at("module")),
    content.at("content").get<picojson::object>()
  };

  Router& router = Router::get_instance();
  router.relay_command(packet, false);
}

/**
 * Send connect_frontend command.
 * @param client Target frontend.
 * @param result Result code.
 * @param my_nid 
 */
void FrontendConnector::send_connect_frontend(uv_pipe_t& client, int result, const nid_t& my_nid) {
  picojson::object data;

  data.insert(std::make_pair("command", picojson::value(std::string("connect_frontend"))));
  data.insert(std::make_pair("result",  picojson::value(static_cast<double>(result))));
  data.insert(std::make_pair("my_nid",  Convert::nid2json(my_nid)));

  send_data(client, data);
}
}  // namespace processwarp
