#include <iostream>
#include "sylar/uri.h"

int main(int argc, char** argv) {
  //  sylar::Uri::ptr uri = sylar::Uri::Create("http://127.0.0.1/test/uri?id=100&name=sylar#frg");
  sylar::Uri::ptr uri = sylar::Uri::Create(
      "http://admin@127.0.0.1/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
  //  sylar::Uri::ptr uri = sylar::Uri::Create("http://admin@127.0.0.1");
  //sylar::Uri::ptr uri = sylar::Uri::Create("http://127.0.0.1/test/uri");
  std::cout << uri->toString() << std::endl;
  auto addr = uri->createAddress();
  std::cout << *addr << std::endl;
  return 0;
}