  

/*

______          _ _      ______ _                       
| ___ \        | (_)     | ___ \ |                      
| |_/ /__ _  __| |_  ___ | |_/ / | __ _ _ __   ___ _ __ 
|    // _` |/ _` | |/ _ \|  __/| |/ _` | '_ \ / _ \ '__|
| |\ \ (_| | (_| | | (_) | |   | | (_| | | | |  __/ |   
\_| \_\__,_|\__,_|_|\___/\_|   |_|\__,_|_| |_|\___|_|   
                                                        
                                                  
                                                   
Description       : RadioPlaner objets.  

License           : Revised BSD License, see LICENSE.TXT file include in the project

Maintainer        : Matthieu Verdy - Fabien Holin (SEMTECH)
*/
#include "RadioPlaner.h"
#include "sx1272.h"
#include "sx1276.h"
#include "SX126x.h"
#include "Define.h"
template class RadioPLaner<SX1276>;
template class RadioPLaner<SX1272>;
template class RadioPLaner<SX126x>;













template <class R> RadioPLaner <R>::RadioPLaner( R * RadioUser) {
    mcu.AttachInterruptIn( &RadioPLaner< R >::CallbackIsrRadioPlaner,this); // attach it radio
    Radio = RadioUser;
    for ( int i = 0 ; i < NB_HOOK; i++ ) { 
        sTask[ i ].HookId         = i;
        sTask[ i ].TaskType       = TASK_IDLE;
        sTask[ i ].StartTime      = 0;
        sTask[ i ].TaskDuration   = 0;
        sTask[ i ].TaskTimingType = TASK_BACKGROUND;
        objHook[ i ]              = NULL;
    }
    HookToExecute = 0xFF;
    RadioPlanerState = RADIO_IDLE;
    PlanerTimerState = TIMER_IDLE;
}
template <class R> RadioPLaner<R>::~RadioPLaner( ) {
}
/************************************************************************************************/
/*                      Public  Methods                                                         */
/************************************************************************************************/
/***********************************************************************************************/
/*                       RadioPlaner Init Method                                               */
/***********************************************************************************************/

template <class R>
eHookStatus RadioPLaner<R>::InitHook ( uint8_t HookIdIn,  void (* AttachCallBack) (void * ), void * objHookIn ) {
    if ( HookIdIn > NB_HOOK ) {
        return ( HOOK_ERROR );
    }
    
    AttachCallBackHook [ HookIdIn ] = AttachCallBack ;
    objHook[ HookIdIn ]             = objHookIn; 
    return ( HOOK_OK );
}

/***********************************************************************************************/
/*                            RadioPlaner GetMyHookId Method                                   */
/***********************************************************************************************/
template <class R>
eHookStatus  RadioPLaner<R>::GetMyHookId  ( void * objHookIn, uint8_t * HookIdIn ){
    for (int i = 0 ; i < NB_HOOK ; i ++) {
        if ( objHookIn == objHook[i] ){
            *HookIdIn = i ;
            return ( HOOK_OK );
        } 
    }
    return (HOOK_ERROR);
}

  
/***********************************************************************************************/
/*                            RadioPlaner EnqueueTask Method                                   */
/***********************************************************************************************/
template <class R> 
void RadioPLaner<R>::EnqueueTask( STask *staskIn, uint8_t *payload, uint8_t *payloadSize, SRadioParam *sRadioParamIn ){
    uint8_t HookId = staskIn->HookId;
    sTask             [ HookId ] = *staskIn;
    sRadioParam       [ HookId ] = *sRadioParamIn;
    Payload           [ HookId ] = payload;
    PayloadSize       [ HookId ] = payloadSize;
    //tb implemented check if already running task and return error
    DEBUG_PRINTF ("                     enqueu task for hook id = %d\n",HookId);
    CallPlanerArbitrer ( );
}


/***********************************************************************************************/
/*                            RadioPlaner GetPlanerStatus Method                               */
/***********************************************************************************************/

template <class R> 
  void RadioPLaner<R>::GetStatusPlaner ( uint32_t * IrqTimestampMs, ePlanerStatus *PlanerStatus ){
    *IrqTimestampMs = IrqTimeStampMs;
    *PlanerStatus   = RadioPlanerStatus;
};


/************************************************************************************************/
/*                      Private  Methods                                                        */
/************************************************************************************************/

/************************************************************************************/
/*                                 Planer Utilities                                 */
/*                                                                                  */
/************************************************************************************/

template <class R>  //@tbd ma   nage all error case
 void RadioPLaner<R> :: ComputePlanerStatus (void ) {   //rename
    uint8_t Id = HookToExecute;
    IrqFlags_t IrqRadio  = ( Radio->GetIrqFlagsLora( ) );
    Radio->ClearIrqFlagsLora( ); 
    switch ( IrqRadio ) {
        case SENT_PACKET_IRQ_FLAG    :
            RadioPlanerStatus = PLANER_TX_DONE ;  
            break;
        case RECEIVE_PACKET_IRQ_FLAG : 
            RadioPlanerStatus = PLANER_RX_PACKET;
            // Push Fifo
            Read_RadioFifo ( sTask[Id].TaskType );
            break; 
        case RXTIMEOUT_IRQ_FLAG      : 
            RadioPlanerStatus = PLANER_RX_TIMEOUT;
            break;
        default:
            break;
    }
}
 
template <class R>     
void RadioPLaner<R>::SetAlarm (uint32_t alarmInMs ) {
    PlanerTimerState = TIMER_BUSY ;
    mcu.StartTimerMsecond( &RadioPLaner<R>::CallbackIsrTimerRadioPlaner,this, alarmInMs);
}

template <class R> 
eHookStatus  RadioPLaner<R>:: Read_RadioFifo ( eRadioPlanerTask  TaskType) {
    eHookStatus status = HOOK_OK;
    uint8_t Id = sCurrentTask.TaskType;
    if (TaskType == TASK_RX_LORA ) {
        Radio->FetchPayloadLora( PayloadSize [ Id ],  Payload [ Id ], sRadioParam[Id].Snr, sRadioParam[Id].Rssi);
    } else if ( TaskType == TASK_RX_FSK ) {
        Radio->FetchPayloadFsk( PayloadSize [ Id ],  Payload [ Id ], sRadioParam[Id].Snr, sRadioParam[Id].Rssi);
    } else {
        status = HOOK_ERROR;
    }
    return status;
}

template <class R> 
void  RadioPLaner<R>::UpdateTaskTab      ( void ) {
    for (int i = 0 ; i < NB_HOOK ; i++ ){
        if ( sTask [ i ].TaskTimingType == TASK_ASSAP ) {
            sTask [ i ].StartTime = mcu.RtcGetTimeMs() + 1; //@note manage timeout 
        }
    }

};
template <class R> 
void  RadioPLaner<R>:: ComputePriority ( void ) {
    for (int i = 0 ; i < NB_HOOK ; i++ ){
        sTask [ i ].Priority = (  sTask [ i ].TaskTimingType * NB_HOOK ) + i ; // the lowest value is the highest priority.
    }

}
template <class R> 
uint8_t  RadioPLaner<R>:: FindHighestPriority ( uint8_t * vec,  uint8_t length ) {
    uint8_t HighPrio = 0xFF ;
    uint8_t Index = 0;
    for (int i = 0 ; i < length ; i++ ){
        if ( vec [ i ] <= HighPrio ) {
            HighPrio = vec [ i ];
            Index = i; 
        }
    }
    return ( Index );
}

template <class R> 
void  RadioPLaner<R>:: ComputeRanking ( void ) { //@tbd implementation should be optimized but very few hooks
    int i;
    uint8_t Index;
    uint8_t RankTemp [NB_HOOK];
    for (int i = 0 ; i < NB_HOOK; i++ ) {
        RankTemp [ i ] =  sTask [ i ].Priority;
    } 
    for (i = 0 ; i < NB_HOOK; i ++) {
        Index = FindHighestPriority ( RankTemp,  NB_HOOK );
        RankTemp [ Index ] = 0xFF;
        Ranking [ i ] = Index ;
    }
}
template <class R> 
uint8_t  RadioPLaner<R>::SelectTheNextTask( void ) {
    DEBUG_MSG ("                                                  call Select Next Task");
    PrintTask ( 0 );
    PrintTask ( 2 );
    uint8_t HookToExecuteTmp = 0xFF;
    uint32_t TimeOfHookToExecuteTmp;
    uint32_t TempTime;
    uint8_t index ;
    int k ;
    for ( k = 0 ; k < NB_HOOK; k++ ) {
        index = Ranking [ k ] ;
        if (sTask[ index ].TaskType < TASK_IDLE) {
            HookToExecuteTmp        = sTask[ index ].HookId ;
            TimeOfHookToExecuteTmp  = sTask[ index ].StartTime ;
            break;
        }
    }
    if (k == NB_HOOK ) {
        return (NO_MORE_TASK);
    }
    for (int i = k ; i < NB_HOOK; i++ ) {
        index = Ranking [ i ] ;
        if ( sTask[ index ].TaskType < TASK_IDLE ) {
            TempTime =  sTask[ index ].StartTime + sTask[ index ].TaskDuration ;
            if ( ( TempTime - TimeOfHookToExecuteTmp ) < 0 ) {   //@relative to avoid issues in case of wrapping
                TimeOfHookToExecuteTmp = sTask[ index ].StartTime ;
                HookToExecuteTmp       = sTask[ index ].HookId ;
            }
        }
    }
    sNextTask = sTask[ HookToExecuteTmp ];
    sTask [ HookToExecuteTmp ].TaskType = TASK_IDLE ;    
    return (SCHEDULED_TASK);
}
/************************************************************************************/
/*                                 Planer Arbiter                                   */
/*                                                                                  */
/************************************************************************************/

template <class R> 
void  RadioPLaner<R>::CallPlanerArbitrer ( void ) {
    UpdateTaskTab      ( );
    ComputePriority    ( );
    ComputeRanking     ( ); 
    uint8_t a = SelectTheNextTask ( );
    if ( SelectTheNextTask() == SCHEDULED_TASK ) { // Store the result in the variable HookToExecut
            LaunchTimer ( );
            DEBUG_MSG ("                                                ");
            DEBUG_PRINTF ("Launch new task for hook id = %d start time at %d\n",HookToExecute, TimeOfHookToExecute);
    } else {
            DEBUG_MSG ("                                                ");
            DEBUG_MSG (" No More Active Task inside the RadioPlaner \n");
    }
    
}

template <class R> 
void  RadioPLaner<R>::LaunchTimer ( void ) { 
    uint32_t CurrentTime = mcu.RtcGetTimeMs () + MARGIN_DELAY ;
    uint32_t delay = sNextTask.StartTime - CurrentTime ;
    switch ( sNextTask.TaskTimingType ) {
        case TASK_AT_TIME :
            if  ( ( delay ) < 0 ) {
                DEBUG_MSG ("                                                ");
                DEBUG_MSG ("TooLATE for NEXT AT TIME TASK ERROR CASE NOT MANAGE YET\n");
                while(1){}
            } else {
                SetAlarm ( delay );
            }
            break;
        case TASK_NOW   :
        case TASK_ASSAP :
                IsrTimerRadioPlaner( ) ;
            break;
        default         :
                DEBUG_MSG ("                                                ");
                DEBUG_MSG ("ERROR UNKNOWN TASK TIMING\n ");
                while(1){}
            break;
    }
}
  


template <class R> 
void  RadioPLaner<R>::LaunchCurrentTask ( void ){
     uint8_t Id = sCurrentTask.HookId;
     switch ( sCurrentTask.TaskType) {
        case TASK_TX_LORA :
            Radio->SendLora( Payload[Id], *PayloadSize[Id], sRadioParam[Id].Sf, sRadioParam[Id].Bw, sRadioParam[Id].Frequency, sRadioParam[Id].Power );
            break;
        case TASK_RX_LORA :
            Radio->RxLora ( sRadioParam[Id].Bw, sRadioParam[Id].Sf, sRadioParam[Id].Frequency, sRadioParam[Id].TimeOutMs);
            break;
        case TASK_TX_FSK :
        case TASK_RX_FSK :
        case TASK_CAD    :
        default :
        break;
    } 
}


/************************************************************************************/
/*                                 Timer Isr Routine                                */
/*                               Called when Alarm expires                          */
/************************************************************************************/

template <class R>     
void RadioPLaner<R>::IsrTimerRadioPlaner( void ) {
    if (sCurrentTask.TaskType < TASK_IDLE) {
                DEBUG_MSG ("                                                ");
                DEBUG_MSG ("Radio already running\n");
                while(1){}
    } else {
        sCurrentTask = sNextTask;
    }
    LaunchCurrentTask ();
    PlanerTimerState = TIMER_IDLE ;
    /*switch  ( sTask[ Id ].TaskType ) {
        case TASK_TX_LORA : 
            RadioPlanerState = RADIO_BUSY ;
            break;
        case TASK_TX_FSK  :
            RadioPlanerState = RADIO_BUSY ; 
            break;
        case TASK_RX_LORA   :
            Radio->RxLora ( sRadioParam[Id].Bw, sRadioParam[Id].Sf, sRadioParam[Id].Frequency, sRadioParam[Id].TimeOutMs);
            RadioPlanerState = RADIO_BUSY ;
            break;
        case TASK_RX_FSK    :
            Radio->RxFsk(sRadioParam[Id].Frequency, sRadioParam[Id].TimeOutMs);
            RadioPlanerState = RADIO_BUSY ;
            break;
        case TASK_CAD :
            RadioPlanerState = RADIO_BUSY ;
            break;
        default :
            RadioPlanerState = RADIO_IDLE;
            break;    
    }*/
}              


/************************************************************************************/
/*                                 Radio Isr Routine                                */
/*                             Called when Radio raises It                          */
/************************************************************************************/
template <class R> 
void  RadioPLaner<R>::IsrRadioPlaner ( void ) {
    IrqTimeStampMs = mcu.RtcGetTimeMs( );
    sCurrentTask.TaskType = TASK_IDLE;
    DEBUG_PRINTF("                                                       receive it for hook %d\n",HookToExecute);
    ComputePlanerStatus ( );
    CallBackHook( sCurrentTask.HookId );
    Radio->Sleep( false );
    CallPlanerArbitrer (); 
} 











/************************************************************************************************/
/*                      DEBUG UTILITIES                                                         */
/************************************************************************************************/
template <class R> 
void  RadioPLaner<R>::PrintTask ( uint8_t HookIdIn ) {
    DEBUG_MSG("\n");
    DEBUG_PRINTF ("                                              Elected Task = %d HookId = %d ",HookToExecute , HookIdIn );
    switch (sTask[HookIdIn].TaskType) {
        case  TASK_RX_LORA : 
            DEBUG_MSG (" TASK_RX_LORA ");
            break;
        case  TASK_RX_FSK :
            DEBUG_MSG (" TASK_RX_FSK ");
            break;
        case  TASK_TX_LORA : 
            DEBUG_MSG (" TASK_TX_LORA ");
            break;
        case  TASK_TX_FSK :
            DEBUG_MSG (" TASK_TX_FSK ");
            break;
        case  TASK_CAD :
            DEBUG_MSG (" TASK_CAD ");
            break;
        case  TASK_IDLE : 
            DEBUG_MSG (" TASK_IDLE ");
            break;
        default :
           DEBUG_MSG (" TASK_ERROR ");
            break;
    };
    DEBUG_PRINTF (" StartTime  @%d with priority = %d",sTask[HookIdIn].StartTime,sTask[HookIdIn].Priority);
    switch (sTask[HookIdIn].TaskTimingType) {
        case  TASK_AT_TIME : 
            DEBUG_MSG (" TASK_AT_TIME ");
            break;
        case  TASK_NOW :
            DEBUG_MSG (" TASK_NOW ");
            break;
        case  TASK_ASSAP : 
            DEBUG_MSG (" TASK_ASSAP ");
            break;
        case  TASK_BACKGROUND : 
            DEBUG_MSG (" TASK_BACKGROUND ");
        default :
           DEBUG_MSG (" TASK_PRIORITY_ERROR ");
    };
    DEBUG_MSG("\n");
}