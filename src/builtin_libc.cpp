
#include <cassert>
#include <cinttypes>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

#include "builtin_libc.hpp"
#include "vmachine.hpp"

using namespace processwarp;

// atexit関数。
bool BuiltinLibc::atexit(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // コピー先アドレスを取得
  vaddr_t func = VMachine::read_builtin_param_ptr(src, &seek);
  // 終了処理時に呼び出す関数一覧
  vm.calls_at_exit.push(func);
  return false;
}

// calloc関数。データ領域の確保とクリアを行う。
bool BuiltinLibc::calloc(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  uint64_t count = VMachine::read_builtin_param_i64(src, &seek);
  uint64_t size = VMachine::read_builtin_param_i64(src, &seek);
  vaddr_t allocated = vm.v_malloc(count * size, false);
  // 領域のクリア
  std::memset(vm.get_raw_addr(allocated), 0, count * size);
  *reinterpret_cast<vaddr_t*>(vm.get_raw_addr(dst)) = allocated;
  return false;
}

// exit関数。
bool BuiltinLibc::exit(VMachine& vm, Thread& th, BuiltinFuncParam p,
			 vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // コピー先アドレスを取得
  uint32_t ret = VMachine::read_builtin_param_i32(src, &seek);
  
  // 終了コードを設定する
  *reinterpret_cast<uint32_t*>(vm.get_raw_addr(th.stackinfos.at(0)->stack)) = ret;
  
  // スタックを1段残して開放する
  while (th.stackinfos.size() > 1) {
    StackInfo& top = *(th.stackinfos.back());
    vm.vmemory.free(top.stack);
    for (vaddr_t addr : top.alloca_addrs) vm.vmemory.free(addr);
    th.stackinfos.pop_back();
  }

  return true;
}

// free関数。指定データ領域を開放する。
bool BuiltinLibc::free(VMachine& vm, Thread& th, BuiltinFuncParam p,
			 vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  vaddr_t ptr = VMachine::read_builtin_param_ptr(src, &seek);
  vm.vmemory.free(ptr);
  return false;
}

// longjmp関数。保存されたスタックコンテキストへの非局所的なジャンプ。
bool BuiltinLibc::longjmp(VMachine& vm, Thread& th, BuiltinFuncParam p,
			    vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // 引数を開く。
  uint8_t* env = vm.get_raw_addr(VMachine::read_builtin_param_ptr(src, &seek));
  uint32_t val = VMachine::read_builtin_param_i32(src, &seek);

  int seek2 = 0;
  // stack_count
  vm_uint_t stack_count = *reinterpret_cast<vm_uint_t*>(env + seek2);
  seek2 += sizeof(vm_int_t);
  // setjmpした時よりスタックが少ない場合エラー
  if (th.stackinfos.size() < stack_count) {
    throw_error(Error::SEGMENT_FAULT);
  }
  
  // 余分なスタックを開放
  while (th.stackinfos.size() > stack_count) {
    const StackInfo& si = *(th.stackinfos.back());
    // スタック領域を解放
    vm.vmemory.free(si.stack);
    // alloca領域を開放
    for (vaddr_t addr : si.alloca_addrs) {
      vm.vmemory.free(addr);
    }
    th.stackinfos.pop_back();
  }
  StackInfo& si = *(th.stackinfos.back());
  // ret_addrのアドレスにvalで指定された値を設定
  *reinterpret_cast<uint32_t*>(vm.get_raw_addr(*reinterpret_cast<vaddr_t*>(env + seek2))) = val;
  seek2 += sizeof(vaddr_t);
  // レジスタの値を戻す。
  // pc
  si.pc = *reinterpret_cast<vm_uint_t*>(env + seek2);
  seek2 += sizeof(vm_uint_t);
  // phi
  si.phi0 = *reinterpret_cast<vm_uint_t*>(env + seek2);
  seek2 += sizeof(vm_uint_t);
  si.phi1 = *reinterpret_cast<vm_uint_t*>(env + seek2);
  seek2 += sizeof(vm_uint_t);
  // type
  si.type = *reinterpret_cast<vaddr_t*>(env + seek2);
  seek2 += sizeof(vaddr_t);
  si.type_cache1 = nullptr;
  si.type_cache2 = nullptr;
  // alignment
  si.alignment = *reinterpret_cast<vm_int_t*>(env + seek2);
  seek2 += sizeof(vm_int_t);
  // output
  si.output = *reinterpret_cast<vaddr_t*>(env + seek2);
  seek2 += sizeof(vaddr_t);
  si.output_cache = nullptr;
  // value
  si.value = *reinterpret_cast<vaddr_t*>(env + seek2);
  seek2 += sizeof(vaddr_t);
  si.value_cache = nullptr;
  // address
  si.address = *reinterpret_cast<vaddr_t*>(env + seek2);
  seek2 += sizeof(vaddr_t);
  si.address_cache = nullptr;

  return true;
}

// malloc関数。データ領域の確保を行う。
bool BuiltinLibc::malloc(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  uint64_t size = VMachine::read_builtin_param_i64(src, &seek);
  *reinterpret_cast<vaddr_t*>(vm.get_raw_addr(dst)) = vm.v_malloc(size, false);
  return false;
}

// memcpy関数。
bool BuiltinLibc::memcpy(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // コピー先アドレスを取得
  vaddr_t p_dst = VMachine::read_builtin_param_ptr(src, &seek);
  // コピー元アドレスを取得
  vaddr_t p_src = VMachine::read_builtin_param_ptr(src, &seek);
  // コピーサイズを取得
  uint64_t p_size = 0;
  switch(p.i64) {
  case 8:  p_size = static_cast<int64_t>(VMachine::read_builtin_param_i8 (src, &seek)); break;
  case 16: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i16(src, &seek)); break;
  case 32: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i32(src, &seek)); break;
  case 64: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i64(src, &seek)); break;

  default: {
    print_debug("p.i64 %" PRIu64 "\n", p.i64);
    assert(false);
  } break;
  }
  // アライメントを取得
  /*int32_t p_align =*/VMachine::read_builtin_param_i32(src, &seek);
  // 実行順番の制約(VMでは実行順番を入れ替えないので無視する)を取得
  /*int8_t p_isvolation =*/VMachine::read_builtin_param_i8(src, &seek);

  // 読み込んだパラメタ長と渡されたパラメタ長は同じはず
  assert(static_cast<signed>(src.size()) == seek);
  std::memcpy(vm.get_raw_addr(p_dst),
	      vm.get_raw_addr(p_src),
	      static_cast<size_t>(p_size));
  return false;
}

// memmove関数。
bool BuiltinLibc::memmove(VMachine& vm, Thread& th, BuiltinFuncParam p,
			    vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // コピー先アドレスを取得
  vaddr_t p_dst = VMachine::read_builtin_param_ptr(src, &seek);
  // コピー元アドレスを取得
  vaddr_t p_src = VMachine::read_builtin_param_ptr(src, &seek);
  // コピーサイズを取得
  uint64_t p_size = 0;
  switch(p.i64) {
  case 8:  p_size = static_cast<int64_t>(VMachine::read_builtin_param_i8 (src, &seek)); break;
  case 16: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i16(src, &seek)); break;
  case 32: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i32(src, &seek)); break;
  case 64: p_size = static_cast<int64_t>(VMachine::read_builtin_param_i64(src, &seek)); break;

  default: {
    print_debug("p.i64 %" PRIu64 "\n", p.i64);
    assert(false);
  } break;
  }
  // アライメントを取得
  /*int32_t p_align =*/VMachine::read_builtin_param_i32(src, &seek);
  // 実行順番の制約(VMでは実行順番を入れ替えないので無視する)を取得
  /*int8_t p_isvolation =*/VMachine::read_builtin_param_i8(src, &seek);

  // 読み込んだパラメタ長と渡されたパラメタ長は同じはず
  assert(static_cast<signed>(src.size()) == seek);
  std::memmove(vm.get_raw_addr(p_dst),
	       vm.get_raw_addr(p_src),
	       static_cast<size_t>(p_size));
  return false;
}

// memset関数。
bool BuiltinLibc::memset(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // 設定先先アドレスを取得
  vaddr_t p_dst = VMachine::read_builtin_param_ptr(src, &seek);
  // 設定値を取得
  uint8_t p_val = VMachine::read_builtin_param_i8(src, &seek);
  // 設定サイズを取得
  uint64_t p_len = 0;
  switch(p.i64) {
  case 8:  p_len = static_cast<int64_t>(VMachine::read_builtin_param_i8 (src, &seek)); break;
  case 16: p_len = static_cast<int64_t>(VMachine::read_builtin_param_i16(src, &seek)); break;
  case 32: p_len = static_cast<int64_t>(VMachine::read_builtin_param_i32(src, &seek)); break;
  case 64: p_len = static_cast<int64_t>(VMachine::read_builtin_param_i64(src, &seek)); break;

  default: {
    print_debug("p.i64 %" PRIu64 "\n", p.i64);
    assert(false);
  } break;
  }
  // アライメントを取得
  /*int32_t p_align =*/VMachine::read_builtin_param_i32(src, &seek);
  // 実行順番の制約(VMでは実行順番を入れ替えないので無視する)を取得
  /*int8_t p_isvolation =*/VMachine::read_builtin_param_i8(src, &seek);
  
  // 読み込んだパラメタ長と渡されたパラメタ長は同じはず
  assert(static_cast<signed>(src.size()) == seek);
  std::memset(vm.get_raw_addr(p_dst), p_val, static_cast<size_t>(p_len));
  return false;
}

// realloc関数。データ領域の再確保を行う。
bool BuiltinLibc::realloc(VMachine& vm, Thread& th, BuiltinFuncParam p,
			    vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  vaddr_t ptr = VMachine::read_builtin_param_ptr(src, &seek);
  uint64_t size = VMachine::read_builtin_param_i64(src, &seek);

  // 新しい領域の確保
  DataStore& new_store = vm.vmemory.alloc_data(size, false);

  if (ptr != VADDR_NULL) {
    // データのコピー
    DataStore& old_store = vm.vmemory.get_data(ptr);
    size_t cp_size = old_store.size < new_store.size ? old_store.size : new_store.size;
    std::memcpy(new_store.head.get(), old_store.head.get(), cp_size);

    // 古い領域の開放
    vm.vmemory.free(ptr);
  }

  *reinterpret_cast<vaddr_t*>(vm.get_raw_addr(dst)) = new_store.addr;
  return false;
}

// VMにライブラリを登録する。
void BuiltinLibc::regist(VMachine& vm) {
  vm.regist_builtin_func("atexit", BuiltinLibc::atexit, 0);
  vm.regist_builtin_func("exit", BuiltinLibc::exit, 0);

  vm.regist_builtin_func("calloc", BuiltinLibc::calloc, 0);
  vm.regist_builtin_func("free", BuiltinLibc::free, 0);
  vm.regist_builtin_func("malloc", BuiltinLibc::malloc, 0);
  vm.regist_builtin_func("realloc", BuiltinLibc::realloc, 0);

  vm.regist_builtin_func("setjmp", BuiltinLibc::setjmp, 0);
  vm.regist_builtin_func("longjmp", BuiltinLibc::longjmp, 0);

  vm.regist_builtin_func("strtol", BuiltinLibc::strtol, 0);
  vm.regist_builtin_func("strtoll", BuiltinLibc::strtol, 0);

  vm.regist_builtin_func("llvm.memcpy.p0i8.p0i8.i8",  BuiltinLibc::memcpy, 8);
  vm.regist_builtin_func("llvm.memcpy.p0i8.p0i8.i16", BuiltinLibc::memcpy, 16);
  vm.regist_builtin_func("llvm.memcpy.p0i8.p0i8.i32", BuiltinLibc::memcpy, 32);
  vm.regist_builtin_func("llvm.memcpy.p0i8.p0i8.i64", BuiltinLibc::memcpy, 64);
  
  vm.regist_builtin_func("llvm.memmove.p0i8.p0i8.i8",  BuiltinLibc::memmove, 8);
  vm.regist_builtin_func("llvm.memmove.p0i8.p0i8.i16", BuiltinLibc::memmove, 16);
  vm.regist_builtin_func("llvm.memmove.p0i8.p0i8.i32", BuiltinLibc::memmove, 32);
  vm.regist_builtin_func("llvm.memmove.p0i8.p0i8.i64", BuiltinLibc::memmove, 64);

  vm.regist_builtin_func("llvm.memset.p0i8.i8",  BuiltinLibc::memset, 8);
  vm.regist_builtin_func("llvm.memset.p0i8.i16", BuiltinLibc::memset, 16);
  vm.regist_builtin_func("llvm.memset.p0i8.i32", BuiltinLibc::memset, 32);
  vm.regist_builtin_func("llvm.memset.p0i8.i64", BuiltinLibc::memset, 64);
}

// setjmp関数。非局所的なジャンプのために、スタックコンテキストを保存する。
bool BuiltinLibc::setjmp(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // 引数を開く。
  uint8_t* env = vm.get_raw_addr(VMachine::read_builtin_param_ptr(src, &seek));
  // 読み込んだパラメタ長と渡されたパラメタ長は同じはず
  assert(static_cast<signed>(src.size()) == seek);

  int seek2 = 0;
  const StackInfo& si = *(th.stackinfos.back());
  // stack_count
  *reinterpret_cast<vm_uint_t*>(env + seek2) = th.stackinfos.size();
  seek2 += sizeof(vm_int_t);
  // ret_addr
  *reinterpret_cast<vaddr_t*>(env + seek2) = dst;
  seek2 += sizeof(vaddr_t);
  // pc
  *reinterpret_cast<vm_uint_t*>(env + seek2) = si.pc + 1;
  seek2 += sizeof(vm_uint_t);
  // phi
  *reinterpret_cast<vm_uint_t*>(env + seek2) = si.phi0;
  seek2 += sizeof(vm_uint_t);
  *reinterpret_cast<vm_uint_t*>(env + seek2) = si.phi1;
  seek2 += sizeof(vm_uint_t);
  // type
  *reinterpret_cast<vaddr_t*>(env + seek2) = si.type;
  seek2 += sizeof(vaddr_t);
  // alignment
  *reinterpret_cast<vm_int_t*>(env + seek2) = si.alignment;
  seek2 += sizeof(vm_int_t);
  // output
  *reinterpret_cast<vaddr_t*>(env + seek2) = si.output;
  seek2 += sizeof(vaddr_t);
  // value
  *reinterpret_cast<vaddr_t*>(env + seek2) = si.value;
  seek2 += sizeof(vaddr_t);
  // address
  *reinterpret_cast<vaddr_t*>(env + seek2) = si.address;
  seek2 += sizeof(vaddr_t);
  
  // setjmp自体の戻り値は0
  *reinterpret_cast<int32_t*>(vm.get_raw_addr(dst)) = 0;
  return false;
}

// strtol関数。文字列を数値に変換する。
bool BuiltinLibc::strtol(VMachine& vm, Thread& th, BuiltinFuncParam p,
			   vaddr_t dst, std::vector<uint8_t>& src) {
  int seek = 0;
  // 引数の読み込み
  vaddr_t nptr = VMachine::read_builtin_param_ptr(src, &seek);
  vaddr_t endptr = VMachine::read_builtin_param_ptr(src, &seek);
  uint32_t base = VMachine::read_builtin_param_i32(src, &seek);

  // 読み込んだパラメタ長と渡されたパラメタ長は同じはず
  assert(static_cast<signed>(src.size()) == seek);

  if (endptr == VADDR_NULL) {
    // endptrがnullの場合は、戻り値をそのまま渡すだけ
    *reinterpret_cast<int64_t*>(vm.get_raw_addr(dst)) =
      std::strtol(reinterpret_cast<char*>(vm.get_raw_addr(nptr)), nullptr, base);

  } else {
    // endptrが指定されている場合、ポインタの書き換えが必要
    char *work_ptr;
    *reinterpret_cast<int64_t*>(vm.get_raw_addr(dst)) =
      std::strtol(reinterpret_cast<char*>(vm.get_raw_addr(nptr)), &work_ptr, base);
    
    if (work_ptr == nullptr) {
      *reinterpret_cast<vaddr_t*>(vm.get_raw_addr(endptr)) = VADDR_NULL;
    } else {
      *reinterpret_cast<vaddr_t*>(vm.get_raw_addr(endptr)) =
	nptr + (reinterpret_cast<uint8_t*>(work_ptr) - vm.get_raw_addr(nptr));
    }
  }
  return false;
}
