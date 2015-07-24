/***************************************************************************
 *   Copyright(C)2009-2015 by zhao_li <lizhao15431230@qq.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
//! \note do not move this pre-processor statement to other places
#include ".\app_cfg.h"

#ifndef __USE_SERVICE_CONSOLE_H__
#define __USE_SERVICE_CONSOLE_H__

/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
//! \brief INSERT_USER_CMD_MAP 
//! @{
//! \brief      user can use this macro to extend static shell command map
//! \Example    {"cmd string",   "help message",cmd_handler}, .....
//! @}

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
//!< \brief shell command strcut
//!< @{
typedef fsm_rt_t shell_handler_t(void *ptCTX, uint8_t chLen);
typedef struct {
    uint8_t *pchCMD;                                //!< shell command string
    uint8_t *pchHelp;                               //!< command help message
    shell_handler_t *fnHandler;                     //!< command handler
}shell_cmd_t;
//!< @}

//! \brief define console sevice interface i_console_t
//! @{
DEF_INTERFACE(i_console_t)
    void        (*Init)     (void);
    fsm_rt_t    (*Task)     (void); 
    struct {
        void    (*Register) (shell_cmd_t *, uint8_t);
        void    (*Remove)   (void);
    } CMD;
END_DEF_INTERFACE(i_console_t)
//! @}

/*============================ GLOBAL VARIABLES ==============================*/
extern const i_console_t CONSOLE;

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/


#endif
