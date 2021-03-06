/* TRANSACT.H   (C) Copyright Bob Wood, 2019-2020                    */
/*                  Transactional-Execution consts and structs       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements the z/Architecture Transactional-Execution */
/* Facility as documented in IBM reference manual SA22-7832-12 "The  */
/* z/Architecture Principles of Operation". Specifically chapter 5   */
/* "Program Execution" pages 5-89 to page 5-109 contain a detailed   */
/* description of the "Transactional-Execution Facility".            */
/*-------------------------------------------------------------------*/

#ifndef _TRANSACT_H_
#define _TRANSACT_H_

/*-------------------------------------------------------------------*/
/*          Transactional-Execution Facility constants               */
/*-------------------------------------------------------------------*/
#define  MAX_TXF_TND             15   /* Max nesting depth           */
#define  MAX_TXF_CONTRAN_INSTR   32   /* Max CONSTRAINED instr.      */
#define  MAX_TXF_PAGES           64   /* Max num of modified pages   */
#define  MAX_CAPTURE_TRIES      128   /* Max clean copy attempts     */

#define  ZPAGEFRAME_PAGESIZE   4096   /* IBM z page size (4K)        */
#define  ZPAGEFRAME_BYTEMASK   0x00000FFF
#define  ZPAGEFRAME_PAGEMASK   0xFFFFFFFFFFFFF000ULL

#define  ZCACHE_LINE_SIZE       256   /* IBM z cache line size       */
#define  ZCACHE_LINE_SHIFT        8   /* Cache line size shift value */
#define  ZCACHE_LINE_PAGE           (ZPAGEFRAME_PAGESIZE/ZCACHE_LINE_SIZE)
                                      /* Cache lines per 4K page     */

/*-------------------------------------------------------------------*/
/*        Transactional-Execution Facility Condition Codes           */
/*-------------------------------------------------------------------*/
#define  TXF_CC_SUCCESS         0   /* Trans. successfully initiated */

#define  TXF_CC_INDETERMINATE   1   /* Indeterminate condition;
                                       successful retry unlikely.    */

#define  TXF_CC_TRANSIENT       2   /* Transient condition;
                                       successful retry likely.      */

#define  TXF_CC_PERSISTENT      3   /* Persistent condition;
                                       successful retry NOT likely
                                       under current conditions. If
                                       conditions change, then retry
                                       MIGHT be more productive.     */

/*-------------------------------------------------------------------*/
/*          abort_transaction function 'retry' code                  */
/*-------------------------------------------------------------------*/
#define  ABORT_RETRY_RETURN     0     /* Return to caller. Used when */
                                      /* abort called from e.g. ext
                                         or I/O interrupt processing */
#define  ABORT_RETRY_CC         1     /* Set condition code and then */
                                      /* longjmp to progjmp.         */
#define  ABORT_RETRY_PGMCHK     2     /* PGMCHK if CONSTRAINED mode, */
                                      /* else longjmp to progjmp.    */

/*-------------------------------------------------------------------*/
/*                   Transaction Page Map                            */
/*-------------------------------------------------------------------*/
struct TPAGEMAP
{
    U64     virtpageaddr;       /* virtual address of mapped page    */
    BYTE*   mainpageaddr;       /* address of main page being mapped */
    BYTE*   altpageaddr;        /* addesss of alternate & save pages */
    BYTE    cachemap[ ZCACHE_LINE_PAGE ];  /* cache line indicators  */

#define CM_CLEAN    0           /* clean cache line (init default)   */
#define CM_FETCHED  1           /* cache line was fetched            */
#define CM_STORED   2           /* cache line was stored into        */
};
typedef struct TPAGEMAP  TPAGEMAP;   // Transaction Page Map table

/*-------------------------------------------------------------------*/
/*                Transaction Diagnostic Block                       */
/*-------------------------------------------------------------------*/
struct TDB
{
    BYTE    tdb_format;         /* Format, 0 = invalid, 1 = valid    */
    BYTE    tdb_flags;          /* Flags                             */

#define TDB_CTV     0x80        /* Conflict-Token Validity           */
#define TDB_CTI     0x40        /* Constrained-Transaction Indicator */

    BYTE    tdb_resv1[4];       /* Reserved                          */
    HWORD   tdb_tnd;            /* Transaction Nesting Depth         */

    DBLWRD  tdb_tac;            /* Transaction Abort Code; see below */
    DBLWRD  tdb_conflict;       /* Conflict token                    */
    DBLWRD  tdb_atia;           /* Aborted-Transaction Inst. Addr.   */

    BYTE    tdb_eaid;           /* Exception Access Identifier       */
    BYTE    tdb_dxc;            /* Data Exception code               */
    BYTE    tdb_resv2[2];       /* Reserved                          */
    FWORD   tdb_piid;           /* Program Interruption Identifier   */

    DBLWRD  tdb_teid;           /* Translation Exception Identifier  */
    DBLWRD  tdb_bea;            /* Breaking Event Address            */
    DBLWRD  tdb_resv3[9];       /* Reserved                          */

    DBLWRD  tdb_gpr[16];        /* General Purpose register array    */

#define TAC_EXT            2    /* External interruption             */
#define TAC_UPGM           4    /* PGM Interruption (Unfiltered)     */
#define TAC_MCK            5    /* Machine-check Interruption        */
#define TAC_IO             6    /* I/O Interruption                  */
#define TAC_FETCH_OVF      7    /* Fetch overflow                    */
#define TAC_STORE_OVF      8    /* Store overflow                    */
#define TAC_FETCH_CNF      9    /* Fetch conflict                    */
#define TAC_STORE_CNF     10    /* Store conflict                    */
#define TAC_INSTR         11    /* Restricted instruction            */
#define TAC_FPGM          12    /* PGM Interruption (Filtered)       */
#define TAC_NESTING       13    /* Nesting Depth exceeded            */
#define TAC_FETCH_OTH     14    /* Cache (fetch related)             */
#define TAC_STORE_OTH     15    /* Cache (store related)             */
#define TAC_CACHE_OTH     16    /* Cache (other)                     */
#define TAC_GUARDED       19    /* Guarded-Storage Event related     */
#define TAC_MISC         255    /* Miscellaneous condition           */
#define TAC_TABORT       256    /* TABORT instruction                */
};
typedef struct TDB  TDB;             // Transaction Dianostic Block

CASSERT( sizeof( TDB ) == 256, transact_h );

/*-------------------------------------------------------------------*/
/*               TXF tracing macros and functions                    */
/*-------------------------------------------------------------------*/

#define TXF_TRACE_UC( _contran )     /* (helper macro) */           \
    (0                                                              \
     || ((sysblk.txf_tracing & TXF_TR_C) &&  (_contran))            \
     || ((sysblk.txf_tracing & TXF_TR_U) && !(_contran))            \
    )

#define TXF_TRACE( _successfailure, _contran )                      \
    (1                                                              \
     && (sysblk.txf_tracing & TXF_TR_ ## _successfailure)           \
     && (TXF_TRACE_UC( _contran ))                                  \
    )

#define TXF_TRACE_TDB( _contran )                                   \
    (1                                                              \
     && (sysblk.txf_tracing & (TXF_TR_FAILURE | TXF_TR_TDB))        \
     && (TXF_TRACE_UC( _contran ))                                  \
    )

#define TXF_TRACE_MAP( _contran )                                   \
    (1                                                              \
     && (sysblk.txf_tracing & TXF_TR_MAP)                           \
     && (TXF_TRACE_UC( _contran ))                                  \
    )

#define TXF_TRACE_PAGES( _contran )                                 \
    (1                                                              \
     && (sysblk.txf_tracing & TXF_TR_PAGES)                         \
     && (TXF_TRACE_UC( _contran ))                                  \
    )

#define TXF_TRACE_LINES( _contran )                                 \
    (1                                                              \
     && (sysblk.txf_tracing & TXF_TR_LINES)                         \
     && (TXF_TRACE_UC( _contran ))                                  \
    )

// Functions to convert a TAC abort code into a printable string
const char*  tac2short ( U64 tac );   // "TAC_INSTR"
const char*  tac2long  ( U64 tac );   // "Restricted instruction"

// Function to hexdump a cache line (HHC17705, HHC17706, HHC17707)
#define DUMP_PFX( _msg )    #_msg "D " _msg
void dump_cache( const char* pfx, int linenum , const BYTE* line);

// Function to hexdump TDB (Transaction Diagnostic Block)
void dump_tdb( TDB* tdb, U64 logical_addr );

#endif // _TRANSACT_H_
