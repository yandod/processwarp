
#include <time.h>

#include "convert.hpp"
#include "llvm_asm_loader.hpp"
#include "server.hpp"

using namespace usagi;

// サーバの繰り返しルーチン。
std::string Server::loop() {
  // VMの処理を進める
  for (Server::VMS::iterator it_vm = vms.begin(); it_vm != vms.end(); it_vm ++) {
    VMachine& vm = *(it_vm->second);
    if (vm.status == VMachine::ACTIVE) {
      vm.execute(10);
    }
  }

  // コマンドを受信する
  std::string& command_str = xmpp.get_command();
  if (command_str != "") {
    picojson::value c;
    // 文字列->JSON
    std::istringstream s(command_str);
    std::string err = picojson::parse(c, s);

    // エラー
    if (!err.empty()) {
      print_debug("command :%s\n", err.c_str());
      return "error";

    } else if (!c.is<picojson::object>()) {
      print_debug("unknown command type.");
      return "error";
    }

    picojson::object cmd_json = c.get<picojson::object>();

    std::string cmd = cmd_json.at("cmd").get<std::string>();
    if (cmd == "ps") {
      command_ps(cmd_json);
	
    } else if (cmd == "warp_in") {
      command_warp_in(cmd_json);

    } else if (cmd == "warp_out") {
      command_warp_out(cmd_json);

    } else {
      print_debug("unknown command : %s", cmd.c_str());
    }
  }

  return "";
}

// サーバのメインルーチンを実行
std::string Server::start(const picojson::object& conf) {
  // ネットワークモードに応じた分岐
  std::string network = conf.at("network").get<std::string>();
  if (network == "xmpp") {
    picojson::object conf_network = conf.at("xmpp").get<picojson::object>();
    xmpp.connect(conf_network.at("server"  ).get<std::string>(),
		 conf_network.at("jid"     ).get<std::string>(),
		 conf_network.at("password").get<std::string>());
    // サーバ名称 = JID
    server_name = conf_network.at("jid").get<std::string>();

  } else {
    return std::string("unknown network mode:") + network;
  }

  // 実行モードに応じた分岐
  std::string mode = conf.at("mode").get<std::string>();
  if (mode == "file") {
    // 指定されたファイルを開く
    picojson::object conf_file = conf.at("file").get<picojson::object>();
    assign_vm(conf_file);
    
  } else if (mode == "none") {
    // 何も起動しないで待機するモード

  } else {
    return std::string("unknown run mode:") + mode;
  }

  return "";
}

// 新規VMを割り当てる。
vpid_t Server::assign_vm(const picojson::object& conf) {
  std::unique_ptr<VMachine> vm(new VMachine());
  vm->setup();

  { // プログラムをロード
    LlvmAsmLoader loader(*vm);
    loader.load_file(conf.at("filename").get<std::string>());
  }
  
  // 機動引数を取り出し
  picojson::array conf_args = conf.at("args").get<picojson::array>();
  std::vector<std::string> args;
  args.push_back(conf.at("filename").get<std::string>());
  for (picojson::array::iterator it = conf_args.begin();
       it != conf_args.end(); it ++) {
    args.push_back(it->get<std::string>());
  }

  // ロードしたプログラムと引数からVMをsetup
  vm->run(args);
  
  // VM固有のIDを作成
  //vpid_t pid = Util::num2hex_str(time(nullptr)) + ";" + server_name;
  fixme("debug用にPIDを固定中");
  vpid_t pid = "00;localhost";

  // 機動成功したVMを一覧に登録
  vms.insert(std::make_pair(pid, std::move(vm)));

  return pid;
}

// サーバで動くプロセス一覧を戻す。
void Server::command_ps(const picojson::object& command) {
  picojson::object reply;

  for (Server::VMS::iterator it_vm = vms.begin(); it_vm != vms.end(); it_vm ++) {
    vpid_t pid = (it_vm->first);
    // VMachine& vm = *(it_vm->second);

    picojson::object vm_info;
    vm_info.insert(std::make_pair("pid", picojson::value(pid)));
    reply.insert(std::make_pair(pid, picojson::value(vm_info)));
  }

  // 応答を送信
  xmpp.send_reply(picojson::value(reply).serialize());
}

// 指定マシンへプロセスを転送する。
void Server::command_warp_in(const picojson::object& command) {
  picojson::object reply;
  picojson::object dump;

  // 対象と送信先を取得
  vpid_t pid     = command.at("pid").get<std::string>();
  std::string to = command.at("to" ).get<std::string>();

  fixme("送信可能かチェックする機能(on_warp_in最中は転送不可など)");

  // 対象VMからスレッドを取得
  VMachine& vm   = *(vms.at(pid).get());
  // LThread& thread = vm.vmemory.get<LThread>(vm.current_thread);

  // VMに関連づいたon_warp_inのメソッドを呼び出す
  //for(vaddr_t cl_addr : vm.on_warp_in) vm.exec_closure(cl_addr);
  
  // 応答JSONを作成
  Convert convert(vm);
  reply.insert(std::make_pair("cmd", picojson::value(std::string("warp_out"))));
  reply.insert(std::make_pair("pid", picojson::value(pid)));
  
  // on_warp_in/on_warp_outをexportする
  //picojson::array on_warp_in;
  //picojson::array on_warp_out;
  //for(auto it : vm.on_warp_in) on_warp_in.push_back(picojson::value(it));
  //for(auto it : vm.on_warp_out) on_warp_out.push_back(picojson::value(it));
  //reply.insert(std::make_pair("on_warp_in",  picojson::value(on_warp_in)));
  //reply.insert(std::make_pair("on_warp_out", picojson::value(on_warp_out)));
  
  Convert::Related related;
  reply.insert(std::make_pair("thread", convert.export_thread(*(vm.threads.back()), related)));
  
  // relatedの中身をすべてexportする
  while(true) {
    bool dump_flg = false;
    for (Convert::Related::iterator it = related.begin(); it != related.end(); it ++) {
      // VM組み込みのアドレスはexportしない
      if (vm.intrinsic_addrs.find(*it) != vm.intrinsic_addrs.end()) continue;

      // 未ダンプなので、ダンプを出力
      if (dump.find(Util::vaddr2str(*it)) == dump.end()) {
	dump_flg = true;
	dump.insert(std::make_pair(Util::vaddr2str(*it), convert.export_store(*it, related)));
	
	break; // relatedが変更された可能性があるのでやり直し
      }
    }

    // すべてスキャンしてdumpしたものがなければ終了
    if (!dump_flg) break;
  }
  reply.insert(std::make_pair("dump", picojson::value(dump)));

  // 応答を送信
  xmpp.send_message(to, picojson::value(reply).serialize());
  
  // スレッドを停止
  vm.status = VMachine::PASSIVE;
}

// 自サーバへ転送されたプロセスをロードする。
void Server::command_warp_out(const picojson::object& command) {
  // picojson::object reply;
  fixme("warp out!!");

  std::string pid = command.at("pid").get<std::string>();

  // VM、変換器を作成
  std::unique_ptr<VMachine> vm(new VMachine());
  vm->setup();
  Convert convert(*vm);

  // 転送されたインスタンスを展開
  picojson::object dump = command.at("dump").get<picojson::object>();
  for (auto it = dump.begin(); it != dump.end(); it ++) {
    convert.import_store(Util::str2vaddr(it->first), it->second);
  }

  // スレッドをと展開
  convert.import_thread(command.at("thread"));
  
  // on_warp_in/on_warp_outをimportする
  //for(auto it : command.at("on_warp_in").get<picojson::array>())
  //vm->on_warp_in.push_back(vmemory.get_addr_by_string(it.get<std::string>(), true));
  //for(auto it : command.at("on_warp_out").get<picojson::array>())
  //vm->on_warp_out.push_back(vmemory.get_addr_by_string(it.get<std::string>(), true));
  
  vm->setup_continuous();

  // アクティブにしたVMを一覧に登録
  vm->status = VMachine::ACTIVE;
  vms.insert(std::make_pair(pid, std::move(vm)));
  
  // VMに関連づいたon_warp_outのメソッドを呼び出す
  //for(vaddr_t cl_addr : vm->on_warp_out) vm->exec_closure(cl_addr);
}
