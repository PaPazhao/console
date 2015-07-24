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
 
#include ".\app_cfg.h"

/*============================ INCLUDES ======================================*/
/*============================ MACROS ========================================*/
#ifndef CMD_MAX_SIZE
#warning macro CMD_MAX_SIZE is not defined,now use default value "16u"
#define CMD_MAX_SIZE                (16u)
#endif

#ifndef PEEK_KEY_MAX_TIME_COUNT
#warning macro PEEK_KEY_MAX_TIME_COUNT is not defined,now use default value "10000u"
#define PEEK_KEY_MAX_TIME_COUNT     (10000u)
#endif

//! \brief user can use this macro to extend static shell command map 
//! \Example    {"cmd string",   "help message",cmd_handler}, .....
#ifndef INSERT_USER_CMD_MAP
#define INSERT_USER_CMD_MAP 
#warning No defined INSERT_USER_CMD_MAP, use default define : none
#endif

#define KEY_NULL                    (0)
#define KEY_DEL                     ('\b')
#define KEY_ESC                     (0x1b)
#define KEY_ENTER                   ('\r')

#define KEY_UP                      (0x82)
#define KEY_DOWN                    (0x83)
#define KEY_RIGHT                   (0x84)
#define KEY_LEFT                    (0x85)

#define KEY_F1                      (0x86)
#define KEY_F2                      (0x87)
#define KEY_F3                      (0x88)
#define KEY_F4                      (0x89)

#define CONSOLE_SEPERATORS          " ;-/"

/*============================ MACROFIED FUNCTIONS ===========================*/
EXTERN_QUEUE(InOutQueue,uint8_t,uint8_t);

/*============================ TYPES =========================================*/
typedef fsm_rt_t console_token_handler_t(uint8_t *pchTokens, uint16_t hwTokens);
typedef fsm_rt_t shell_handler_t(void *ptCTX, uint8_t chLen);

//! \brief define history command struct
//! @{
typedef struct {
    uint8_t     chHeadIndex;    //!< current history head pointer
    uint8_t     chTailIndex;    //!< current history Tail pointer
    uint8_t     chCmdCount;     //!< current history command number
    uint8_t     chPeekIndex;    //!< current peek command index
    uint8_t     chPeekCount;    //!< current peek count
}histroy_cmd_t;
//! @}

//!< \brief shell command strcut
//!< @{
typedef struct {
    uint8_t *pchCMD;            //!< shell command string
    uint8_t *pchHelp;           //!< command help message
    shell_handler_t *fnHandler; //!< command handler
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

/*============================ PROTOTYPES ====================================*/
/*! \brief console_low_init 
 *! 
 *! \param void 
 *! 
 *! \retval none
 */
WEAK void console_low_init(void);

/*! \brief console task init
 *! 
 *! \param none
 *! 
 *! \retval none
 */
static void console_init(void);

/*! \brief console task
 *! 
 *! \param none
 *! 
 *! \retval fsm_rt_cpl 
 */
static fsm_rt_t console_task(void);

/*! \note console dynamic command remove
 *！
 *! \param none
 *！
 *! \retval none  
 */
static void console_cmd_remove(void);

/*! \note console dynamic command register
 *！
 *! \param ptCMD register command list
 *! \param chCount register command conut
 *！
 *! \retval none  
 */
static void console_cmd_register(shell_cmd_t* ptCMD, uint8_t chCount);

/*! \brief front_end 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_front_end(void);

/*---------------------------  console task top--------------------------*/
/*! \brief parase 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_parase(void);

/*! \brief shell 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_shell(void);

/*---------------------------  console task frontend--------------------------*/
/*! \brief printer 
 *! 
 *! \param pchBuf print text buffer pointer 
 *! \param chNum  print text buffer length 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
fsm_rt_t console_print(const uint8_t *pchBuf,uint8_t chNum);

/*! \brief Key Checker 
 *! 
 *! \param pchKeyValue key value 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_key_check(uint8_t *pchKeyValue);

/*! \brief front_end  command line editor
 *! 
 *! \param chKeyCode func key or char
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_editor(uint8_t chKeyCode);

/*! \brief enter handler 
 *! 
 *! \param none
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_enter(void);

/*! \brief histoyr manager 
 *! 
 *! \param chKeyCode functiong key value
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_history_manager(uint8_t chKeyCode);

/*! \brief get key value
 *! 
 *! \param pchKeyCode func key value
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t get_key_value(uint8_t *pchKeyCode);

/*! \brief peek form FiFoIn
 *! 
 *! \param pchByte      key value
 *! \param wDelayCount   timeout count
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t peek_byte_with_timeout(uint8_t *pchByte ,uint32_t wDelayCount);

/*! \brief register appliction handler
 *! 
 *! \param ptHandler     applictin hander function
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static void register_app_handler(console_token_handler_t *ptHandler);

/*! \brief appliction handler---print tokens one token one line
 *! 
 *! \param pchTokens     tokens buffer
 *! \param hwTokens     tokens count
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t tokens_print_handler(uint8_t *pchTokens, uint16_t hwTokens);

/*! \brief check seperator
 *! 
 *! \param chbyte 
 *! 
 *! \retval true chbyte is seperator
 *! \retval false chbyte is not seperator
 */
static bool seperator_checker(uint8_t chbyte);

/*---------------------------  command buffer line ----------------------------*/
//!< @{
/*! \brief initialize command line buffer
 *! 
 *! \param none
 *! 
 *! \retval none 
 */
static void cmd_buffer_init(void);

/*! \brief add command char to command buffer
 *! 
 *! \param ptCTX command char pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_add_char(void *ptCTX);

/*! \brief delete command char 
 *! 
 *! \param ptCTX none
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_del_char(void *ptCTX);

/*! \brief append command string to command buffer
 *! 
 *! \param ptCTX command line pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_append_string(void *ptCTX);

/*! \brief clear command line 
 *! 
 *! \param ptCTX none
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_line_clear(void);

/*! \brief refresh command line buffer
 *! 
 *! \param ptCTX command line pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_refresh(void *ptCTX);
//!< @}
                                           
/*---------------------------  history command manger --------------------------*/
//!< @{
/*! \brief initialize history maneager
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval none 
 */
static void history_cmd_init(void);

/*! \brief and new command to history command
 *! 
 *! \param pchCmdPTR command line buffer pointer
 *! \param chCMDlen  command line length
 *! 
 *! \retval none 
 */
static void history_add_new_cmd(uint8_t* pchCmdPTR,uint8_t chCMDlen);

/*! \brief get the latest history command
 *! 
 *! \param none
 *! 
 *! \retval command pointer 
 */
static uint8_t* history_get_latest_new(void);

/*! \brief search next history 
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval uint8_t* history command pointer
 */
 
static uint8_t* search_next_cmd(void);

/*! \brief search last history command
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval uint8_t* history command pointer
 */
static uint8_t* search_last_cmd(void);

/*---------------------------   message map   --------------------------*/

/*! \brief commpare string
 *! 
 *! \param pchString1 string 1 pointer
 *! \param pchString2 string 2 pointer
 *! 
 *! \retval true if string1 == string2 return ture 
 *! \retval false if string1 != string2 return false 
 */
static bool str_cmp(uint8_t *pchString1, uint8_t *pchString2);

/*! \brief search shell command map 
 *! 
 *! \param pchTokens tokens buffer
 *! 
 *! \retval msg pointer
 */
static const shell_cmd_t* search_cmd(uint8_t* pchTokens);

/*! \brief check if pchString meeting "help" or "h" 
 *! 
 *! \param pchString string pointer
 *! 
 *! \retval true meeting help string
 *! \retval false not find help sting
 */
static bool check_help(uint8_t *pchString);

/*! \brief print command help message
 *! 
 *! \param ptCMD message pointer
 *! 
 *! \retval fsm_rt_on_going  fsm is running
 *! \retval fsm_rt_cpl  fsm is compelted
 */
static fsm_rt_t print_cmd_help_message(shell_cmd_t* ptCMD);

/*! \brief clear command handler , clear screen
 *! 
 *! \param ptCTX no used
 *! 
 *! \retval fsm_rt_on_going  fsm is running
 *! \retval fsm_rt_cpl  fsm is compelted
 */
static fsm_rt_t clear_handler(void *ptCTX, uint8_t chLen);

/*! \brief help command handler , output all userfull command
 *! 
 *! \param ptCTX no used
 *! 
 *! \retval fsm_rt_on_going  fsm is running
 *! \retval fsm_rt_cpl  fsm is compelted
 */
static fsm_rt_t help_handler(void *ptCTX, uint8_t chLen);

/*! \brief string upper to lower 
 *! 
 *! \param pchString string buffer
 *! \param chMaxLen  max string len
 *! 
 *! \retval none
 */
static void string_to_lower(uint8_t *pchString, uint8_t chMaxLen);
//!< @}

/*============================ LOCAL VARIABLES ===============================*/
//!< \brief command string
//!< @{
const static uint8_t c_chRN[]      = {'\r','\n'};
const static uint8_t c_chpropmt[]  = {'\r','\n','>'};
const static uint8_t c_chDelChar[] = {'\b',' ','\b'};
//!< @}

//!< \brief error shell command print message
//!< @{
const static uint8_t c_chErrNoMSG[] = " command not found! \r\n";
//!< @}
            
//!< \brief command buffer
//!< @{
static uint8_t s_chCmdBuf[CMD_MAX_SIZE];
static uint8_t s_chCmdBufIndex;
//!< @}

//!< \brief printer control
//!< @{
static uint8_t s_chNum  = 0;
static uint8_t *s_pchPRT = NULL;
//!< @}

//!< \brief dynamic command
//!< @{
static shell_cmd_t *s_ptDynaCMD = NULL;
static uint8_t s_chDynaCount = 0;
//!< @}

//!< \brief tokens buffer
//!< @{
static uint8_t s_chTokenBuff[CMD_MAX_SIZE];
static uint16_t s_hwTokensNum = 0;
//!< @}

//!< \brief history manager
//!< @{
#if USE_CONSOLE_HISTORY_CMD == ENABLED
#   ifndef HISTTORY_CMD_MAX_COUNT
#       warning macro HISTTORY_CMD_MAX_COUNT is not defined,now use default value "1u"
#       define HISTTORY_CMD_MAX_COUNT       (4u)
#   endif
static uint8_t  s_chHistroyCMDBuff[HISTTORY_CMD_MAX_COUNT][CMD_MAX_SIZE];   //!< history command buffer
static histroy_cmd_t s_tHistoryManger;
#else
static uint8_t  s_chHistroyCMDBuff[1][CMD_MAX_SIZE];                        //!< history command buffer
#endif
//!< @}

//!< \brief key_value
//! @{
//ESC   :   1B
//UP    :   1B 5B 41
//DOWN  :   1B 5B 42
//LEFT  :   1B 5B 44
//RIGHT :   1B 5B 43
    
//F1    :   1B 4F 50
//F2    :   1B 4F 51
//F3    :   1B 4F 52
//F4    :   1B 4F 53
const static uint8_t c_chFunKeyCode[] = {   0x5b,
                                            0x4f,
                                            0x41,   //2    UP
                                            0x42,   //3    DOWN
                                            0x43,   //4    RIGHT
                                            0x44,   //5    LEFT
                                            0x50,   //6    F1
                                            0x51,   //7    F2
                                            0x52,   //8    F3
                                            0x53};  //9    F4
//! @}

static console_token_handler_t *s_ptParaserHandler = NULL;

//!< \brief build-in shell command map
const static shell_cmd_t s_tStaticMap[] = {
    {"clear",   " clear\t--\tClear Screen\r\n",                                  clear_handler},
    {"help",    " help\t--\tGet command list of all available commands.\r\n",    help_handler},
    INSERT_USER_CMD_MAP
};
//!< \brief help command string
static uint8_t *s_cHelpString[] = {"h", "help"};

/*============================ GLOBAL VARIABLES ==============================*/
//!< \brief inout stream 
//! @{
extern QUEUE(InOutQueue) g_tFIFOin;
extern QUEUE(InOutQueue) g_tFIFOout;
//! @}

//!< \brief console service interface.
//! @{
const i_console_t CONSOLE = {
    .Init           =   &console_init,
    .Task           =   &console_task,
    .CMD            =   {
        .Register   =   &console_cmd_register,
        .Remove     =   &console_cmd_remove,
    }
};
//! @}

/*===============================  Console_task_top  ==========================*/
/*! \brief console_low_init 
 *! 
 *! \param void 
 *! 
 *! \retval none
 */
static void console_low_init(void)
{
    return;
}

/*! \brief console task init
 *! 
 *! \param none
 *! 
 *! \retval none
 */
static void console_init(void)
{
    //!< console task lower init.
    console_low_init();
    
    //!< init histroy command manager class
    history_cmd_init();
    
    //!< register app handler -- print tokens
    //register_app_handler(tokens_print_handler);
}

/*! \brief console task
 *! 
 *! \param none
 *! 
 *! \retval fsm_rt_cpl 
 */
static fsm_rt_t console_task(void)
{
    #define TASK_COSOLE_RESET() do{s_tState = TASK_CONSOLE_START;} while(0)
    static enum {
        TASK_CONSOLE_START = 0,
        TASK_CONSOLE_FRONT_END,
        TASK_CONSOLE_PARSE,
        TASK_CONSOLE_USERAPP
    }s_tState = TASK_CONSOLE_START;
    
    switch(s_tState) {
        case TASK_CONSOLE_START:
            cmd_buffer_init();
            s_tState = TASK_CONSOLE_FRONT_END;
            //break;
            
        case TASK_CONSOLE_FRONT_END:            
            if( fsm_rt_cpl == console_front_end() ) {   //!< front_end
                s_tState = TASK_CONSOLE_PARSE;
            }
            break;
            
        case TASK_CONSOLE_PARSE:            
            if( fsm_rt_cpl == console_parase() ){       //!< parase
                s_tState = TASK_CONSOLE_USERAPP;
            }
            break;
        
        case TASK_CONSOLE_USERAPP:            
            if( fsm_rt_cpl == console_shell() ) {       //!< shelll
                TASK_COSOLE_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \note console dynamic command register
 *！
 *! \param ptCMD register command list
 *! \param chCount register command conut
 *！
 *! \retval none  
 */
static void console_cmd_register(shell_cmd_t* ptCMD, uint8_t chCount)
{
    if ((NULL == ptCMD) || (!chCount)){
        return ;
    }
    
    s_ptDynaCMD = ptCMD;
    s_chDynaCount = chCount;
    
    while(chCount--) {
        string_to_lower((uint8_t *)(ptCMD++),CMD_MAX_SIZE);
    }
}

/*! \note console dynamic command remove
 *！
 *! \param none
 *！
 *! \retval none  
 */
static void console_cmd_remove(void)
{
    s_ptDynaCMD = NULL;
    s_chDynaCount = 0;
}

/*===============================  Front end  =================================*/
/*! \brief front_end 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_front_end(void)
{
    #define CONSOLE_FRONT_END_RESET()   do {s_tState = CONSOLE_FRONT_END_START;} while(0)
    static uint8_t s_chKeyValue ;
    static enum {
        CONSOLE_FRONT_END_START = 0,
        CONSOLE_FRONT_END_PRINT_PROMPT,
        CONSOLE_FRONT_END_KEY_CHECKER,
        CONLOLE_FRONT_HISTORY_MANGER,
        CONSOLE_FRONT_END_EDITOR,
        CONSOLE_FRONT_END_ENTER
    }s_tState = CONSOLE_FRONT_END_START;
    
    switch(s_tState) {
        case CONSOLE_FRONT_END_START:
            s_tState = CONSOLE_FRONT_END_PRINT_PROMPT;
            //break;
            
        case CONSOLE_FRONT_END_PRINT_PROMPT:
            if(fsm_rt_cpl == console_print(c_chpropmt,UBOUND(c_chpropmt))) {    //!< print ">"
                s_tState = CONSOLE_FRONT_END_KEY_CHECKER;
            }
            break;

        case CONSOLE_FRONT_END_KEY_CHECKER:
            if(fsm_rt_cpl == console_key_check(&s_chKeyValue) ){                //!< key checker
                switch(s_chKeyValue) {
                    case KEY_UP :
                    case KEY_DOWN :
                    case KEY_F1 :
                    case KEY_F3 :
                        case KEY_LEFT:
                        case KEY_RIGHT:
                        s_tState = CONLOLE_FRONT_HISTORY_MANGER;                    //!< KEY : F1 / F3 / up / down
                        break;
                        
                    case KEY_ENTER :
                        s_tState = CONSOLE_FRONT_END_ENTER;                         //!< KEY : Enter
                        break;
                        
                    case KEY_DEL:
                        s_tState = CONSOLE_FRONT_END_EDITOR;
                        break;

                    default:
                        if((s_chKeyValue > 31) && (s_chKeyValue < 127) ){           //!< char / KEY : delete
                            s_tState = CONSOLE_FRONT_END_EDITOR;
                        }
                        break;
                }
            }
            break;
            
        case CONSOLE_FRONT_END_EDITOR:
            if(fsm_rt_cpl == console_editor(s_chKeyValue) ){                    //!< key editor
                s_tState = CONSOLE_FRONT_END_KEY_CHECKER;
            }
            break;

            
        case CONLOLE_FRONT_HISTORY_MANGER:
            if(fsm_rt_cpl == console_history_manager(s_chKeyValue) ){           //!< history manger
                s_tState = CONSOLE_FRONT_END_KEY_CHECKER;
            }
            break; 
            
         case CONSOLE_FRONT_END_ENTER:
             if(fsm_rt_cpl == console_enter() ){                                  //!< enter handler
                 CONSOLE_FRONT_END_RESET();
                 return fsm_rt_cpl;
             }
            break; 
    }
    
    return fsm_rt_on_going;
}

/*! \brief printer 
 *! 
 *! \param pchBuf print text buffer pointer 
 *! \param chNum  print text buffer length 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
//static fsm_rt_t console_print(const uint8_t *pchBuf,uint8_t chNum)
fsm_rt_t console_print(const uint8_t *pchBuf,uint8_t chNum)
{
    #define CONSOLE_PRT_RESET()   do {s_tState = CONSOLE_PRT_START;} while(0)
    static uint8_t s_chPrintIndex ;
    static enum {
        CONSOLE_PRT_START = 0,
        CONSOLE_PRT_PRINT_OVER,
        CONSOLE_PRT_PRINT
    }s_tState = CONSOLE_PRT_START;
    
    if((NULL == pchBuf) || (!chNum)) {
        return fsm_rt_err;
    }
    
    switch(s_tState) {
        case CONSOLE_PRT_START:
            s_chPrintIndex = 0;
            s_tState = CONSOLE_PRT_PRINT_OVER;
            //break;
            
        case CONSOLE_PRT_PRINT_OVER:
            if(s_chPrintIndex < chNum) {
                s_tState = CONSOLE_PRT_PRINT;
            } else {
                CONSOLE_PRT_RESET();
                return fsm_rt_cpl;
            }
            //break;
            
        case CONSOLE_PRT_PRINT:
            if(ENQUEUE(InOutQueue,&g_tFIFOout,pchBuf[s_chPrintIndex])) {
                s_tState = CONSOLE_PRT_PRINT_OVER;
                s_chPrintIndex++;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \brief Key Checker 
 *! 
 *! \param pchKeyValue key value 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_key_check(uint8_t *pchKeyValue)
{
    #define COSOLE_CHECK_RESET()        do{s_tState = CONSOLE_CHECK_START;} while(0)
    static enum {
        CONSOLE_CHECK_START = 0,
        CONSOLE_CHECK_GET_KEY_VALUE,
    }s_tState;
    
    switch(s_tState) {
        case CONSOLE_CHECK_START:
            if(NULL == pchKeyValue ) {
                return fsm_rt_err;
            }
            s_tState = CONSOLE_CHECK_GET_KEY_VALUE;
            //break;
            
        case CONSOLE_CHECK_GET_KEY_VALUE:
            if(fsm_rt_cpl == peek_byte_with_timeout(pchKeyValue,PEEK_KEY_MAX_TIME_COUNT)) {
                if(fsm_rt_cpl == get_key_value(pchKeyValue)) {
                    if(*pchKeyValue) {
                        COSOLE_CHECK_RESET();
                        return fsm_rt_cpl;
                    } 
                }                
            }
            break;

    }
    
    return fsm_rt_on_going;
}

/*! \brief  command line editor
 *! 
 *! \param chKeyCode func key or char
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_editor(uint8_t chKeyCode)
{
    #define EDITOR_RESET() do {s_tState = EDITOR_START;} while(0)
    typedef fsm_rt_t editor_handler_t(void *pchKey);

    static editor_handler_t *s_ptKeyHandler = NULL;
    static enum {
        EDITOR_START = 0,
        EDITOR_KEY_HANDLER,
    }s_tState = EDITOR_START;
    
    switch(s_tState) {
        case EDITOR_START:
            if(KEY_DEL == chKeyCode) {
                s_ptKeyHandler = cmd_buffer_del_char;
            } else {
                s_ptKeyHandler = cmd_buffer_add_char;
            }
            s_tState = EDITOR_KEY_HANDLER;
            //break;
            
        case EDITOR_KEY_HANDLER:
            if(fsm_rt_cpl == s_ptKeyHandler(&chKeyCode)) {
                EDITOR_RESET();
                return fsm_rt_cpl;
            }
    }
    
    return fsm_rt_on_going;
}

/*! \brief histoyr manager 
 *! 
 *! \param chKeyCode functiong key value
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_history_manager(uint8_t chKeyCode)
{
    #define MANAGER_RESET() do {s_tState = MANAGER_START;} while(0)
    
    typedef fsm_rt_t history_manager_handler_t(void *pchKey);

    static history_manager_handler_t *s_ptHistoryHandler;
    static uint8_t *s_pchCTX = NULL;
    static enum {
        MANAGER_START = 0,
        MANAGER_HANDLER,
    }s_tState = MANAGER_START;
    
    
    switch(s_tState) {
        case MANAGER_START:
            s_ptHistoryHandler = NULL;

            switch(chKeyCode) {
                case KEY_F1:
                case KEY_RIGHT:
                    s_pchCTX = history_get_latest_new();
                    if(s_chCmdBufIndex < (uint8_t)strlen(s_pchCTX)){
                        s_ptHistoryHandler = cmd_buffer_add_char;
                        s_pchCTX = s_pchCTX + s_chCmdBufIndex;
                    }
                    break;
                     
                case KEY_LEFT:
                case KEY_F3:
                    s_pchCTX = history_get_latest_new();
                    if(s_chCmdBufIndex < strlen(s_pchCTX)) {
                        s_pchCTX = s_pchCTX + s_chCmdBufIndex;
                        s_ptHistoryHandler = cmd_buffer_append_string;
                    }
                    break;

                case KEY_UP:
                    s_pchCTX = search_last_cmd(); 
                    if(s_pchCTX) {
                        s_ptHistoryHandler = cmd_buffer_refresh;
                    }
                    break;

                case KEY_DOWN:
                    s_pchCTX = search_next_cmd(); 
                    if(s_pchCTX) {
                        s_ptHistoryHandler = cmd_buffer_refresh;
                    }
                    break;
            }
            
            if(NULL == s_ptHistoryHandler) {
                MANAGER_RESET();
                return fsm_rt_cpl;
            }
            
            s_tState = MANAGER_HANDLER;
            break;
            
        case MANAGER_HANDLER:
            if(fsm_rt_cpl == s_ptHistoryHandler(s_pchCTX)) {
                MANAGER_RESET();
                return fsm_rt_cpl;
            }
    }
    
    return fsm_rt_on_going;
}

/*! \brief enter handler 
 *! 
 *! \param none
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_enter(void)
{
    #define CONSOLE_ENTER_RESET() do {s_tState = CONSOLE_ENTER_START;} while(0)
    static enum {
        CONSOLE_ENTER_START = 0,
        CONSOLE_ENTER_PRINT_RN,
    }s_tState = CONSOLE_ENTER_START;
    
    switch(s_tState) {
        case CONSOLE_ENTER_START: 
            //!< add cmd buffer to history command
            history_add_new_cmd(s_chCmdBuf,strlen(s_chCmdBuf));
            s_tState = CONSOLE_ENTER_PRINT_RN;
            //break;
            
        case CONSOLE_ENTER_PRINT_RN:
            if(fsm_rt_cpl == console_print(c_chRN,UBOUND(c_chRN))) {    //!< print return 
                CONSOLE_ENTER_RESET();
                return fsm_rt_cpl;
            }
    }
    
    return fsm_rt_on_going;
}

/*! \brief peek form FiFoIn
 *! 
 *! \param pchByte      key value
 *! \param wDelayCount   timeout count
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t peek_byte_with_timeout(uint8_t *pchByte ,uint32_t wDelayCount)
{
    #define PEEK_BYTE_RESET() do {s_tState = PEEK_BYTE_START;} while(0)
    static uint32_t s_wCount;
    static enum {
        PEEK_BYTE_START = 0,
        PEEK_BYTE_CHECK
    }s_tState = PEEK_BYTE_START;
    
    switch(s_tState) {
        case PEEK_BYTE_START:
            if( NULL == pchByte) {
                return fsm_rt_err;
            }
            *pchByte = 0;
            s_wCount = wDelayCount;
            s_tState = PEEK_BYTE_CHECK;
            //break;
            
        case PEEK_BYTE_CHECK:
            if((PEEK_QUEUE(InOutQueue, &g_tFIFOin, pchByte)) || (!s_wCount)){
                PEEK_BYTE_RESET();
                return fsm_rt_cpl;
            }
            s_wCount--;
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \brief get key value
 *! 
 *! \param pchKeyCode func key value
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t get_key_value(uint8_t *pchKeyCode)
{
    #define FUNC_KEY_CHECK_RESET() do { s_tState = FUNC_KEY_CHECK_START; } while(0)
    static uint8_t s_chIndex ;
    static enum {
        FUNC_KEY_CHECK_START = 0,
        FUNC_KEY_CHECK_HEAD,
        FUNC_KEY_CHECK_SECOND,
        FUNC_KEY_CHECK_THIRD,
    }s_tState = FUNC_KEY_CHECK_START;
    
    switch(s_tState) {
        case FUNC_KEY_CHECK_START:
            if ( NULL == pchKeyCode) {
                return fsm_rt_err;
            }
            s_tState = FUNC_KEY_CHECK_HEAD;
            //break;
    
        case FUNC_KEY_CHECK_HEAD:
            if (0x1b == *pchKeyCode) {                  //!< head
                s_chIndex = 0;
                s_tState = FUNC_KEY_CHECK_SECOND;
            } else {
                if (*pchKeyCode) {                      //!< key != NULL
                    DEQUEUE(InOutQueue,&g_tFIFOin,pchKeyCode);
                }
                
                FUNC_KEY_CHECK_RESET();
                return fsm_rt_cpl;                
            }
            break;
  
        case FUNC_KEY_CHECK_SECOND:
            do {
                if( *pchKeyCode == c_chFunKeyCode[s_chIndex]) {
                    s_tState = FUNC_KEY_CHECK_THIRD;
                    s_chIndex = 2 + ( s_chIndex << 2);
                    return fsm_rt_on_going;
                }
            } while(++s_chIndex < 2);
                            
            RESET_PEEK(InOutQueue,&g_tFIFOin);

            DEQUEUE(InOutQueue,&g_tFIFOin,pchKeyCode);
            *pchKeyCode = KEY_ESC;
            FUNC_KEY_CHECK_RESET();
            return fsm_rt_cpl;
            
        case FUNC_KEY_CHECK_THIRD:{
            uint8_t chIndex = 0;
            do {
                if( *pchKeyCode == c_chFunKeyCode[s_chIndex]) {
                    *pchKeyCode = s_chIndex + 0x80;
                    GET_ALL_PEEK(InOutQueue,&g_tFIFOin);
                    FUNC_KEY_CHECK_RESET();
                    return fsm_rt_cpl;
                }
                s_chIndex++;
            } while(++chIndex < 4);
            RESET_PEEK(InOutQueue,&g_tFIFOin);

            DEQUEUE(InOutQueue,&g_tFIFOin,pchKeyCode);
            *pchKeyCode = KEY_ESC;
            FUNC_KEY_CHECK_RESET();
            return fsm_rt_cpl;
        }
    }

    return fsm_rt_on_going;
}

/*===============================  Parase =====================================*/
/*! \brief parase 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_parase(void)
{
    #define CONSOLE_PARASER_RESET() do {s_tState = CONSOLE_PARASE_START;} while(0)
    static enum {
        CONSOLE_PARASE_START = 0,
        CONSOLE_PARASE_GET_TOKENS,
        CONSOLE_PARASE_PARASE_HANDLER
    }s_tState = CONSOLE_PARASE_START;

    static bool s_bHeaderFlag = false;

    if(s_tState == CONSOLE_PARASE_START) {
        s_hwTokensNum = 0;
        s_tState = CONSOLE_PARASE_GET_TOKENS;
    } else if (s_tState == CONSOLE_PARASE_GET_TOKENS) {
        uint16_t hwCmdLineIndex = 0;
        uint16_t hwTokenIndex = 0;
        uint8_t chTemp;
        
        s_bHeaderFlag = false;
        do {
            chTemp = s_chCmdBuf[hwCmdLineIndex];                //!< ger char from command line buffer
            
            if(chTemp == '\0')  {
                s_chTokenBuff[hwTokenIndex] = 0;
                break;
            }
            
            if(!s_bHeaderFlag) {
                if(!seperator_checker(chTemp)) {                     //!< chTemp != separator
                    s_hwTokensNum++;
                    s_bHeaderFlag = true;
                    s_chTokenBuff[hwTokenIndex++] = chTemp;
                }
            } else {
                if(seperator_checker(chTemp)) {                     //!< chTemp == separator
                    s_chTokenBuff[hwTokenIndex++] = 0;
                    s_bHeaderFlag = false;
                } else {
                    s_chTokenBuff[hwTokenIndex++] = chTemp;
                }
            }
            
            if(++hwCmdLineIndex == CMD_MAX_SIZE - 1) {
                s_chTokenBuff[hwTokenIndex] = 0;
                break;
            }
        }while(1);
        
        if((NULL == s_ptParaserHandler) || (!s_hwTokensNum) ) { //!< tokens is empty or paraser handler no registor,break;
            CONSOLE_PARASER_RESET();
            return fsm_rt_cpl;
        }
        
        s_tState = CONSOLE_PARASE_PARASE_HANDLER;
        
    } else if (s_tState == CONSOLE_PARASE_PARASE_HANDLER) {
        if(fsm_rt_cpl == s_ptParaserHandler(s_chTokenBuff, s_hwTokensNum)) {
            CONSOLE_PARASER_RESET();
            return fsm_rt_cpl;
        }
    }
    
    return  fsm_rt_on_going;
}

/*! \brief check seperator
 *! 
 *! \param chbyte 
 *! 
 *! \retval true chbyte is seperator
 *! \retval false chbyte is not seperator
 */
static bool seperator_checker(uint8_t chbyte)
{
    uint8_t* pchSeperator = CONSOLE_SEPERATORS;

    while(*pchSeperator != '\0') {
        if(chbyte == *pchSeperator++) {
            return true;
        }
    }
    
    return false;
}

/*===============================  Shell ====================================*/
/*! \brief help command handler -- print all shell command help meassage 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t help_handler(void *ptCTX, uint8_t chLen)
{
    #define CMD_HELP_HANDER_RESET_FSM() do {s_tState = CMD_HELP_HANDER_START;} while(0)
    static uint8_t s_chCount,s_chDynaIndex;
    static  shell_cmd_t* s_ptPTR ;
    static enum {
        CMD_HELP_HANDER_START = 0,
        CMD_HELP_HANDER_GET,
        CMD_HELP_PRT
    }s_tState = CMD_HELP_HANDER_START;
    
    switch (s_tState) {
        case CMD_HELP_HANDER_START:
            s_chCount = 0;
            s_chDynaIndex = 0;
            s_ptPTR = NULL;
            s_tState = CMD_HELP_HANDER_GET;
            //!< break;
                
        case CMD_HELP_HANDER_GET:
            if (s_chCount < UBOUND(s_tStaticMap)) {
                s_ptPTR = (shell_cmd_t*)&s_tStaticMap[s_chCount++];
            } else {
                if (s_chDynaIndex < s_chDynaCount) {
                    s_ptPTR = &s_ptDynaCMD[s_chDynaIndex++];
                } else {
                    CMD_HELP_HANDER_RESET_FSM();
                    return fsm_rt_cpl;
                }
            }
            s_tState = CMD_HELP_PRT;
            break;
            
        case CMD_HELP_PRT:
            if(fsm_rt_cpl == print_cmd_help_message(s_ptPTR)) {
                s_tState = CMD_HELP_HANDER_GET;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \brief shell 
 *! 
 *! \param void 
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t console_shell(void)
{
    #define SHELL_RESET() do {s_State = SHELL_START;} while(0)
//     static uint8_t *s_ptToken;
    static const shell_cmd_t* s_ptCMD;
    static enum {
        SHELL_START = 0,
        SHELL_TOKEN_CHECK,
        SHELL_CHECK_HELP,
        SHELL_CMD_HELP,
        SHELL_PRT_ERR_NO_MSG,
        SHELL_MSG_HANDLER
    }s_State = SHELL_START;
    
    switch(s_State) {
        case SHELL_START:
            s_ptCMD = NULL;
            s_State = SHELL_TOKEN_CHECK;
            //break;
            
        case SHELL_TOKEN_CHECK:
            if(s_hwTokensNum) {
                s_ptCMD = search_cmd(s_chTokenBuff);
                if(s_ptCMD) {
                    if(s_hwTokensNum > 1) {
                        s_State = SHELL_CHECK_HELP;
                    } else {
                        s_State = SHELL_MSG_HANDLER;
                    }
                } else {
                    s_State = SHELL_PRT_ERR_NO_MSG;
                }
            } else {
                SHELL_RESET();
                return fsm_rt_cpl;
            }
            break;
    
        case SHELL_CHECK_HELP:
            if(check_help(s_chTokenBuff + 1 + strlen(s_chTokenBuff))) {
                s_State = SHELL_CMD_HELP;
            } else {
                s_State = SHELL_MSG_HANDLER;
            }
            break;
            
        case SHELL_CMD_HELP:
            if(fsm_rt_cpl == print_cmd_help_message((shell_cmd_t*)s_ptCMD)) {                           //!< shell command help handler
                SHELL_RESET();
                return fsm_rt_cpl;
            }
            break;
            
        case SHELL_PRT_ERR_NO_MSG:
            if(fsm_rt_cpl == console_print(c_chErrNoMSG,UBOUND(c_chErrNoMSG))) {        //!< print "command not found!"
                SHELL_RESET();
                return fsm_rt_cpl;
            }
            break;
            
/*        case SHELL_PRT_ERR_NO_HELP:
            if(fsm_rt_cpl == console_print(c_chErrNoHelp,UBOUND(c_chErrNoHelp))) {      //!< print "help: no help topics match!"
                SHELL_RESET();
                return fsm_rt_cpl;
            }
            break;*/
            
        case SHELL_MSG_HANDLER:{                                                        //!< shell command handler
            void *ptCTX = NULL;
            
            if(s_hwTokensNum > 1) {
                ptCTX = (void *)( s_chTokenBuff + 1 + strlen(s_chTokenBuff));
            }

            if (!s_ptCMD->fnHandler(ptCTX, s_hwTokensNum-1)) {
                SHELL_RESET();
                return fsm_rt_cpl;
            }
            break;
        }

    }

    return fsm_rt_on_going;
}

static bool check_help(uint8_t *pchString)
{
    uint8_t chIndex = 0;
    
    if(!pchString) {
        return false;
    }
    
    while (chIndex < UBOUND(s_cHelpString)) {
        if (str_cmp(pchString, s_cHelpString[chIndex])) {
            return true;
        } else {
            chIndex++;
        }
    }
    
    return false;
}

/*! \brief register appliction handler
 *! 
 *! \param ptHandler     applictin hander function
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static void register_app_handler(console_token_handler_t *ptHandler)
{
    if(ptHandler) {
        s_ptParaserHandler = ptHandler;
    }
}

/*! \brief appliction handler---print tokens one token one line
 *! 
 *! \param pchTokens     tokens buffer
 *! \param hwTokens     tokens count
 *! 
 *! \retval fsm_rt_on_going  running
 *! \retval fsm_rt_cpl       compelte
 */
static fsm_rt_t tokens_print_handler(uint8_t *pchTokens, uint16_t hwTokens)
{
    #define TOKENS_PRINT_HANDLER_RESET() do { s_tState = TOKEN_PRINT_START; } while(0)
    static uint8_t *s_ptToken;
    static uint16_t s_hwTokensCount;
    static enum {
        TOKEN_PRINT_START = 0,
        TOKEN_PRINT_CHECK,
        TOKEN_PRINT_PRT_TOKEN,
        TOKEN_PRINT_PRT_RN
    }s_tState = TOKEN_PRINT_START;
    
    switch(s_tState) {
        case TOKEN_PRINT_START:
            if(( NULL == pchTokens) || ( !hwTokens )) {
                return fsm_rt_err;
            }
            s_ptToken = pchTokens;
            s_hwTokensCount = hwTokens;
            s_tState = TOKEN_PRINT_CHECK;
            //break;
            
        case TOKEN_PRINT_CHECK:                                             //!< get token from tokens buffer
            if(s_hwTokensCount) {
                s_hwTokensCount--;
                s_tState = TOKEN_PRINT_PRT_TOKEN; 
            } else {
                TOKENS_PRINT_HANDLER_RESET();
                return fsm_rt_cpl;
            }
            break;
            
        case TOKEN_PRINT_PRT_TOKEN:{
                uint8_t chTokenLen = strlen(s_ptToken);

                if(fsm_rt_cpl == console_print(s_ptToken, chTokenLen)) {    //!< print token
                    s_ptToken = (uint8_t *)( s_ptToken + 1 + chTokenLen);
                    s_tState = TOKEN_PRINT_PRT_RN;
                }
            }
            break;
        case TOKEN_PRINT_PRT_RN:
            if(fsm_rt_cpl == console_print(c_chRN,UBOUND(c_chRN))) {        //!< print return 
                s_tState = TOKEN_PRINT_CHECK;
            }
            break;
    }
    return fsm_rt_on_going;
}

/*===============================  command buffer line =========================*/
//!< @{

/*! \brief initialize command line buffer
 *! 
 *! \param none
 *! 
 *! \retval none 
 */
static void cmd_buffer_init(void)
{
    s_chCmdBufIndex = 0;
    s_chCmdBuf[0] = 0;
}

/*! \brief add command char to command buffer
 *! 
 *! \param ptCTX command char pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_add_char(void *ptCTX)
{
    #define ADD_CHAR_RESET() do {s_tState = ADD_CHAR_START;} while(0)
    static enum {
        ADD_CHAR_START = 0,
        ADD_CHAR_CHECK_FULL,
        ADD_CHAR_START_ECO
    }s_tState = ADD_CHAR_START;
    
    switch(s_tState) {
        case ADD_CHAR_START:
            if(NULL == ptCTX) {
                return fsm_rt_cpl;
            }
            s_tState = ADD_CHAR_CHECK_FULL;
            //break;
            
        case ADD_CHAR_CHECK_FULL:
            if(s_chCmdBufIndex >= CMD_MAX_SIZE - 1) {            //!< add char to command line buffer
                ADD_CHAR_RESET();
                return fsm_rt_cpl;
            }
            
            s_pchPRT = &s_chCmdBuf[s_chCmdBufIndex];
            s_chNum = 1;

            s_chCmdBuf[s_chCmdBufIndex++] = *(uint8_t*)ptCTX;
            s_chCmdBuf[s_chCmdBufIndex] = '\0';
            
            s_tState = ADD_CHAR_START_ECO;
            break;
            
        case ADD_CHAR_START_ECO:                                        //!< echo 
            if(fsm_rt_cpl == console_print(s_pchPRT,s_chNum)) {
                ADD_CHAR_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
            
    return fsm_rt_on_going;
}

/*! \brief delete command char 
 *! 
 *! \param ptCTX none
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_del_char(void *ptCTX)
{
    #define DEL_CHAR_RESET() do {s_tState = DEL_CHAR_START;} while(0)
    static enum {
        DEL_CHAR_START = 0,
        DEL_CHAR_CHECK_EMPTY,
        DEL_CHAR_PRT_STRING
    }s_tState = DEL_CHAR_START;
    
    switch(s_tState) {
        case DEL_CHAR_START:
            s_tState = DEL_CHAR_CHECK_EMPTY;
            //break;
        case DEL_CHAR_CHECK_EMPTY:                                          //!< adjust command line buffer index
            if(s_chCmdBufIndex == 0) {
                DEL_CHAR_RESET();
                return fsm_rt_cpl;
            }
            s_chCmdBufIndex--;
            s_chCmdBuf[s_chCmdBufIndex] = '\0';

            s_pchPRT = (uint8_t*)&c_chDelChar;
            s_chNum = UBOUND(c_chDelChar);
            s_tState = DEL_CHAR_PRT_STRING;        
            break;
            
        case DEL_CHAR_PRT_STRING:                                           //!< echo del command
            if(fsm_rt_cpl == console_print(s_pchPRT,s_chNum)) {
                DEL_CHAR_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
            
    return fsm_rt_on_going;
}

/*! \brief append command string to command buffer
 *! 
 *! \param ptCTX command line pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_append_string(void *ptCTX)
{
    #define APPEND_CMD_RESET() do {s_tState = APPEND_CMD_LINE_START;} while(0)
    static enum {
        APPEND_CMD_LINE_START = 0,
        APPEND_CMD_LINE_COPY_CMD,
        APPEND_CMD_LINE_ECHO_CMD
    }s_tState = APPEND_CMD_LINE_START;
    
    switch(s_tState) {
    
        case APPEND_CMD_LINE_START:
            if(NULL == ptCTX){
                return fsm_rt_err;
            }
            s_tState = APPEND_CMD_LINE_COPY_CMD;
            //break;
        
        case APPEND_CMD_LINE_COPY_CMD:                                                  //!< append string to command
            s_chNum = MIN(strlen(ptCTX), CMD_MAX_SIZE - s_chCmdBufIndex - 1);
            memcpy(&s_chCmdBuf[s_chCmdBufIndex],ptCTX,s_chNum);
            s_pchPRT = &s_chCmdBuf[s_chCmdBufIndex];
            
            s_chCmdBufIndex += s_chNum;
            s_chCmdBuf[s_chCmdBufIndex] = '\0';
            s_tState = APPEND_CMD_LINE_ECHO_CMD;
            break;
            
        case APPEND_CMD_LINE_ECHO_CMD:                                                  //!< echo append string
            if(fsm_rt_cpl == console_print(s_pchPRT,s_chNum)) {
                APPEND_CMD_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \brief refresh command line buffer
 *! 
 *! \param ptCTX command line pointer
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_buffer_refresh(void *ptCTX)
{
    #define REFRESH_CMD_LINE_RESET() do {s_tState = REFRESH_CMD_LINE_START;} while(0)
    static enum {
        REFRESH_CMD_LINE_START = 0,
        REFRESH_CMD_LINE_CLR_CMD,
        REFRESH_CMD_LINE_ECHO_CMD
    }s_tState = REFRESH_CMD_LINE_START;

    switch(s_tState) {
        case REFRESH_CMD_LINE_START:
            if(NULL == ptCTX) {
                return fsm_rt_err;
            }
            s_tState = REFRESH_CMD_LINE_CLR_CMD;
            //break;
        
        case REFRESH_CMD_LINE_CLR_CMD:
            if(fsm_rt_cpl == cmd_line_clear()) {                    //!< clear commmand line
                //s_chNum = MIN(strlen(ptCTX), CMD_MAX_SIZE);
                //s_pchPRT = s_chCmdBuf;
                //
                ////!< copy command line to command buffer
                //strncpy(s_chCmdBuf,ptCTX,CMD_MAX_SIZE);
                s_tState = REFRESH_CMD_LINE_ECHO_CMD;
            }
            break;
            
        case REFRESH_CMD_LINE_ECHO_CMD:
            if(fsm_rt_cpl == cmd_buffer_append_string(ptCTX)) {         //!< append string to command buffer
                REFRESH_CMD_LINE_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \brief clear command line 
 *! 
 *! \param ptCTX none
 *! 
 *! \retval fsm_rt_cpl finesh 
 *! \retval fsm_rt_on_going state running 
 */
static fsm_rt_t cmd_line_clear(void)
{
    #define CLEAR_CMD_LINE_RESET() do {s_tState = CLEAR_CMD_LINE_START;} while(0)
    static enum {
        CLEAR_CMD_LINE_START = 0,
        CLEAR_CMD_LINE_CHECK_OVER,
        CLEAR_CMD_LINE_DEL_CHAR,
    }s_tState = CLEAR_CMD_LINE_START;
    
    switch(s_tState) {
        case CLEAR_CMD_LINE_START:
            s_tState = CLEAR_CMD_LINE_CHECK_OVER;
            //break;
        
        case CLEAR_CMD_LINE_CHECK_OVER:
            if(s_chCmdBufIndex == 0) {
                CLEAR_CMD_LINE_RESET();
                return fsm_rt_cpl;
            } else {
                s_tState = CLEAR_CMD_LINE_DEL_CHAR;
            }
            break;
            
         case CLEAR_CMD_LINE_DEL_CHAR:
            if(fsm_rt_cpl == cmd_buffer_del_char(NULL)){
                s_tState = CLEAR_CMD_LINE_CHECK_OVER;
            }
            break;
    }
    
    return fsm_rt_on_going;
}
//!< @}

/*===============================  history command manger ======================*/
#if USE_CONSOLE_HISTORY_CMD == ENABLED
//!< @{
/*! \brief initialize history cmd object
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval none 
 */
static void history_cmd_init(void)
{
    s_tHistoryManger.chHeadIndex  = 0;
    s_tHistoryManger.chTailIndex  = 0;
    s_tHistoryManger.chCmdCount   = 0;
    s_tHistoryManger.chPeekIndex  = 0;
    s_tHistoryManger.chPeekCount  = 0;
}

/*! \brief search next history command
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval uint8_t* history command pointer
 */
static uint8_t* search_next_cmd(void)
{
    if( s_tHistoryManger.chPeekCount > 1 ) {
    
        s_tHistoryManger.chPeekIndex++;
        if(HISTTORY_CMD_MAX_COUNT == s_tHistoryManger.chPeekIndex){
            s_tHistoryManger.chPeekIndex = 0;
        }
        
        s_tHistoryManger.chPeekCount--;
        return s_chHistroyCMDBuff[s_tHistoryManger.chPeekIndex];
    } 

    return NULL;
}

/*! \brief search last history command
 *! 
 *! \param ptOBJ history cmd object pointer
 *! 
 *! \retval uint8_t* history command pointer
 */
static uint8_t* search_last_cmd(void)
{
    if( s_tHistoryManger.chPeekCount < s_tHistoryManger.chCmdCount ) {
        if( 0 == s_tHistoryManger.chPeekIndex ) {
            s_tHistoryManger.chPeekIndex = HISTTORY_CMD_MAX_COUNT;
        }
        s_tHistoryManger.chPeekIndex--;
        s_tHistoryManger.chPeekCount++;
        
        return s_chHistroyCMDBuff[s_tHistoryManger.chPeekIndex];
    }

    return NULL;
}


/*! \brief search last history command
 *! 
 *! \param pchCmdPTR history command  pointer
 *! \param chCMDlen history command  length
 *! 
 *! \retval uint8_t* history command pointer
 */
static void history_add_new_cmd(uint8_t* pchCmdPTR,uint8_t chCMDlen)
{
    uint8_t* pchPTR = NULL;
    
    if((NULL == pchCmdPTR) || (!chCMDlen)) {
        return;
    }
    
    if(chCMDlen > CMD_MAX_SIZE) {
        return;
    }
    
    pchPTR = s_chHistroyCMDBuff[s_tHistoryManger.chTailIndex];

    memcpy(pchPTR,pchCmdPTR,chCMDlen+1);
    
    if(++(s_tHistoryManger.chTailIndex) >= HISTTORY_CMD_MAX_COUNT) {
        s_tHistoryManger.chTailIndex = 0;
    }
    
    if( s_tHistoryManger.chCmdCount == HISTTORY_CMD_MAX_COUNT) {
        s_tHistoryManger.chHeadIndex = s_tHistoryManger.chTailIndex;
    } else {
        s_tHistoryManger.chCmdCount++;
    }

    s_tHistoryManger.chPeekIndex = s_tHistoryManger.chTailIndex;
    s_tHistoryManger.chPeekCount = 0;
}

/*! \brief get latest history commmand address
 *! 
 *! \param void
 *! 
 *! \retval uint8_t* latest history commmand address pointer
 */
static uint8_t* history_get_latest_new(void)
{
    if(s_tHistoryManger.chCmdCount) {
        if(s_tHistoryManger.chTailIndex) {
            return s_chHistroyCMDBuff[s_tHistoryManger.chTailIndex - 1];
        } else {
            return s_chHistroyCMDBuff[HISTTORY_CMD_MAX_COUNT - 1];
        }
    }
    return NULL;
}

/*! \brief string upper to lower 
 *! 
 *! \param pchString string buffer
 *! \param chMaxLen  max string len
 *! 
 *! \retval none
 */
static void string_to_lower(uint8_t *pchString, uint8_t chMaxLen)
{
    uint16_t chLen;

    chLen = strlen(pchString);

    if (chLen > chMaxLen) {
        chLen = chMaxLen;
    }

    if (chLen) {
        while (chLen--) {
            uint8_t chTemp = *pchString;
            if ((chTemp > '@') && (chTemp < '[')) { //!< upper to lower
                *pchString = chTemp + 'a' - 'A';
            }
            pchString++;
        }
    }
}

/*! \brief search shell command map 
 *! 
 *! \param pchTokens tokens buffer
 *! 
 *! \retval msg pointer
 */
static const shell_cmd_t* search_cmd(uint8_t* pchTokens)
{
    uint8_t chCount = 0;

    if(!pchTokens) {
        return NULL;
    }
    
    string_to_lower(pchTokens,CMD_MAX_SIZE);    //!< tonens upper to lower
    
    while (chCount < UBOUND(s_tStaticMap)) {
        if (str_cmp(pchTokens, s_tStaticMap[chCount].pchCMD)) {
            //*pchSize = str_len(s_tStaticMap[chCount].pchCMD) + 1;
            return &s_tStaticMap[chCount];
        } else {
            chCount++;
        }
    }
    
    chCount = 0;
    while (chCount < s_chDynaCount) {
        if (str_cmp(pchTokens, s_ptDynaCMD[chCount].pchCMD)) {
            return &s_ptDynaCMD[chCount];
        } else {
            chCount++;
        }
    }

    return NULL;
}

/*! \note clear command handler
 *！
 *! \param none
 *！
 *! \retval fsm_rt_on_going  fsm is running  
 *! \retval fsm_rt_cpl  fsm is complete  
 */
static fsm_rt_t clear_handler(void *ptCTX, uint8_t chLen)
{
    return console_print("\x0c",1);
}

static fsm_rt_t print_cmd_help_message(shell_cmd_t* ptCMD)
{
    #define PRT_CMD_HELP_RESET() do {s_tState = PRT_CMD_START;} while(0)
    static enum {
        PRT_CMD_START = 0,
        PRT_CMD_STRING,
        PRT_CMD_HELP_MESSAGE
    }s_tState = PRT_CMD_START;
    
    switch(s_tState) {
        case PRT_CMD_START:
            s_tState = PRT_CMD_HELP_MESSAGE;
            break;
            
//         case PRT_CMD_STRING:
//             if(fsm_rt_cpl == console_print(ptCMD->pchCMD,strlen(ptCMD->pchCMD))) {      //!< print shell command string
//                 s_tState = PRT_CMD_HELP_MESSAGE;
//             }
//             break;

        case PRT_CMD_HELP_MESSAGE:
            if(fsm_rt_cpl == console_print(ptCMD->pchHelp,strlen(ptCMD->pchHelp))) {    //!< print shell command help message
                PRT_CMD_HELP_RESET();
                return fsm_rt_cpl;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

/*! \note string compare
 *!
 *! \param pchString1
 *! \param pchString2
 *!
 *! \retval true pchString1 == pchString2
 *! \retval false pchString1 != pchString2
 */
static bool str_cmp(uint8_t *pchString1, uint8_t *pchString2)
{
    while (*pchString1 == *pchString2) {
        if ('\0' == *pchString1) {
            return true;
        } else {
            pchString1++;
            pchString2++;
        }
    }
    
    return false;
}
//!< @}
#endif













