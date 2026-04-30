// RUN: %check_clang_tidy %s lenovo-sec004-unsafe-deserialization %t

namespace boost {
namespace archive {
class binary_iarchive {
public:
  binary_iarchive(int) {}
};
class text_iarchive {
public:
  text_iarchive(int) {}
};
}
}

void Bad1() {
  boost::archive::binary_iarchive ar(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:35: warning: use of unsafe deserialization API 'binary_iarchive'; validate or migrate to a sandboxed format [lenovo-sec004-unsafe-deserialization]
  (void)ar;
}

void Bad2() {
  boost::archive::text_iarchive ar(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:33: warning: use of unsafe deserialization API 'text_iarchive'; validate or migrate to a sandboxed format [lenovo-sec004-unsafe-deserialization]
  (void)ar;
}
