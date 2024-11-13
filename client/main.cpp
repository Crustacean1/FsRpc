#define FUSE_USE_VERSION 31
//
#include "./client.h"
#include <fuse.h>
#include <sys/syslog.h>

const fuse_operations xmp_oper = {.getattr = client::getattr,
                                  .unlink = nullptr,
                                  .chmod = nullptr,
                                  .open = client::open,
                                  .read = client::read,
                                  .write = client::write,
                                  .readdir = client::readdir,
                                  .init = client::init,
                                  .destroy = client::destroy,
                                  .lseek = nullptr};

int main(int argc, char **argv) {
  openlog("rpc-client", LOG_PID | LOG_CONS, LOG_USER);
  int result = fuse_main(argc, argv, &xmp_oper, NULL);
  return result;
}
