
#include "inc/spinapi.h"
#include "src/caps.h"

extern BOARD_INFO board[];
extern int cur_board;

SPINCORE_API int
ovj_get_radioprocessor (void)
{
   return board[cur_board].is_radioprocessor;
}

