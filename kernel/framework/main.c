#include <kernel.h>
#include <klib.h>
#include <common.h>
int main() {
  _ioe_init();
  _cte_init(os->trap);
  //_vme_init(pmm->alloc, pmm->free);
  os->init();
  assert(0);
  _mpe_init(os->run);
  return 1;
}
