// shsl is distributed under the GNU LGPL v2.1 (see LICENSE file for details)
// author: Alonzo Bazaar <alonzo.lo.stronzo@protonmail.com>
// (that is indeed a pseudonym)

#define SHSL_IMPLEMENTATION
#include "shsl.h"

// #define SHSL_EXEC_IMPLEMENTATION
// #include "shsl_exec.h"

// #define SHSL_FS_IMPLEMENTATION
// #include "shsl_fs.h"

int main(int argc, char** argv) {
    /*
      shsl_ref env = shsl_mkempty_env();
      shsl_add_std_defs(env);
      // shsl_add_exec_defs(env);
      // shsl_add_fs_defs(env);
      return shsl_main(env, argc, argv);
     */
    return shsl_main(argc, argv);
}
