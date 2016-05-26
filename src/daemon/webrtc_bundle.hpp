#pragma once

#include <uv.h>

#include <webrtc/api/peerconnectioninterface.h>
#include <webrtc/api/test/fakeconstraints.h>
#include <webrtc/base/flags.h>
#include <webrtc/base/physicalsocketserver.h>
#include <webrtc/base/ssladapter.h>
#include <webrtc/base/thread.h>

#include <map>
#include <memory>
#include <string>

#include "packet.hpp"
#include "type.hpp"
#include "webrtc_connector.hpp"

namespace processwarp {
class WebrtcBundle : public WebrtcConnectorDelegate, public PacketControllerDelegate {
 public:
  static WebrtcBundle& get_instance();

  void apply_connector(WebrtcConnector* connector);
  WebrtcConnector* create_connector();
  void finalize();
  void initialize(uv_loop_t* loop);
  void relay(const Packet& packet);
  void set_nid(const NodeID& nid);
  void purge_connector(WebrtcConnector* connector);
  void relay_init_webrtc_ice(const NodeID& local_nid, const NodeID& remote_nid,
                             const std::string& ice);
  void relay_init_webrtc_offer(const NodeID& prime_nid, const std::string& sdp);

 private:
  /** Main loop of libuv. */
  uv_loop_t* loop;
  /** Sub-thread instance of libuv. */
  uv_thread_t subthread;

  /** Event loop of webrtc on sub-thread. */
  rtc::Thread* thread;
  /** WebRTC socket server instance. */
  rtc::PhysicalSocketServer socket_server;

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
  webrtc::PeerConnectionInterface::RTCConfiguration pc_config;
  webrtc::DataChannelInit dc_config;

  PacketController packet_controller;
  std::map<WebrtcConnector*, std::unique_ptr<WebrtcConnector>> connectors;
  /** Map of connector and node-id that relaied the packet from new node. */
  std::map<WebrtcConnector*, NodeID>  init_map;

  NodeID my_nid;
  NodeID next_minus_nid;
  NodeID next_plus_nid;
  NodeID range_min_nid;
  NodeID range_max_nid;

  WebrtcBundle();
  WebrtcBundle(const WebrtcBundle&);
  WebrtcBundle& operator=(const WebrtcBundle&);
  virtual ~WebrtcBundle();

  void packet_controller_on_recv(const Packet& packet) override;
  void packet_controller_send(const Packet& packet) override;

  void webrtc_connector_on_change_stateus(WebrtcConnector& connector, bool is_connect) override;
  void webrtc_connector_on_update_ice(WebrtcConnector& connector,
                                      const std::string ice) override;

  static void subthread_entry(void* arg);

  void recv_init_webrtc_ice(const Packet& packet);
  void recv_init_webrtc_offer(const Packet& packet);
  void relay_to_another(const Packet& packet);
  void relay_to_local(const Packet& packet);
  void send_packet_error(const Packet& error_for, PacketError::Type code);
};
}  // namespace processwarp