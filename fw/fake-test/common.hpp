int prt(int a) {
  // std::cout << "running: " << a << std::endl;
  auto ret = 0;
  if (a == 1) {
    ret += 10;
  } else if (a == 1000) {
    ret += 9999;
  } else {
    ret += a;
  }
  return ret;
}
