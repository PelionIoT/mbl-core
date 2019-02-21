/*
 * Copyright (c) 2019 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DBusAdapter_Impl_h_
#define _DBusAdapter_Impl_h_

#include "MblError.h"
#include "Mailbox.h"
#include "CloudConnectExternalTypes.h"
#include "EventManager.h"

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include <inttypes.h>
#include <pthread.h>
#include <set>
#include <string>

// sd-bus vtable object, implements the com.mbed.Cloud.Connect1 interface
// Keep those definitions here for testing
#define DBUS_CLOUD_SERVICE_NAME                 "com.mbed.Cloud"
#define DBUS_CLOUD_CONNECT_INTERFACE_NAME       "com.mbed.Cloud.Connect1"
#define DBUS_CLOUD_CONNECT_OBJECT_PATH          "/com/mbed/Cloud/Connect1"

namespace mbl {

/**
 * @brief DBusAdapter Implementation class
  * Implements the CCRB thread event loop, publishes and maintains the D-Bus service, supplies
  * inter-thread communication services and handles all the lower layer operations needed to 
  * send/receive D-Bus messages.
  * It holds :
  * 1) references to all asynchronouse messages waiting for replay.
  * 2) event manager to schedule self immidiate/periodic/delayed events.
  * 3) Mailbox(es) to communicate with external threads.
  * 4) A refernce to ccrb to invoke its API (mainly on message arrival)
  * 5) Hold an API to be called be CCRB (init/deinit/start/stop invoke events)
  * 6) Set of callbacks to implemented  sd-bus/sd-event callabacks.
 */
class DBusAdapterImpl
{
// Google Test friend class - used to access private members from test body
friend ::TestInfra_DBusAdapterTester;
private:
    // reference to the Cloud Conect Resource Broker (CCRB) which acts as the upper layer and the 
    // containing object for 
    ResourceBroker &ccrb_;

    // TODO : (when upper layer code is more complete) 
    // consider set <atomic> + add conditional variable in order to 
    // synchronize upper layer init/deinit in a simple way
    /**
     * @brief - DBusAdapterImpl current state (not initialized , initialized (stopped) or running)
     * simple container used to easily preform basic operations on the current state 
     * in a more encapsulated manner. This allows clear logging to the state.
     * member names are self-explaining
     */
    class State {
    public:
        enum class eState {
            UNINITALIZED, 
            INITALIZED,         
            RUNNING,
        };        

        // Basic operations
        const char*     to_string();
        void            set(eState new_state);
        State::eState   get(); 
        bool            is_equal(eState state);
        bool            is_not_equal(eState state);
        
    private:
        eState current_ = eState::UNINITALIZED;
    };       
    State state_;
    
    /*
    === Callback and handler functions ===

    Callbacks are static (called by systemd C code) and might do some common processing.
    Afterwards, callback continues always by static-cast into in a non-static object 
    specific member function. 
    
    The same parameters are used in all callbacks and handlers, unless stated with a full 
    * description. The parameters name are reserved name by the sd-bus/sd-event implementations.
    * 
    * @param m - handle to the message received
    * @param userdata  - userdata supplied while calling sd_bus_attach_event() - always 'this' 
    * @param ret_error - The sd_bus_error structure carries information about a D-Bus error 
    * condition.
    * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
    */

    /**
     * @brief - called by the event loop when a D-Bus message received
     * preforms basic checks and then calls a message-specific processing function.
     * see parameter description above ("Callback and handler functions")
     */
    static int incoming_bus_message_callback(
       sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

    /**
     * @brief - process incoming RegisterResources message. parse JSON file and send it to CCRB.
     * validates that sender published agreed D-Bus API.
     * receives an answer from CCRB and accordingly, sends back method reply / error reply to 
     * sender.
     * see parameter description above ("Callback and handler functions")
     */
    int process_message_RegisterResources(sd_bus_message *m, sd_bus_error *ret_error);        

    /**
     * @brief - TODO
     * see parameter description above ("Callback and handler functions")
     */
    int process_message_DeregisterResources(sd_bus_message *m, sd_bus_error *ret_error);

    /**
     * @brief - TODO
     * see parameter description above ("Callback and handler functions")
     * For more information see :
     * https://www.freedesktop.org/software/systemd/man/sd_bus_add_match.html#
     * 
     */
    static int name_changed_match_callback(
        sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
    /**
     * @brief - TODO
     * see parameter description above ("Callback and handler functions")
     * for more information see 
     * https://www.freedesktop.org/software/systemd/man/sd_bus_add_match.html#
     */
    int name_changed_match_callback_impl(
        sd_bus_message *m, sd_bus_error *ret_error);

    /**
     * @brief handles incoming mailbox message sent by an external thread - the actual 
     * implementation (as member) is done by calling incoming_mailbox_message_callback_impl
     * @param s - the sd-event IO event source
     * @param fd - the Linux anonymous pipe file descriptor used to poll&read from the mailbox 
     * pipe
     * For more information see :
     * https://www.freedesktop.org/software/systemd/man/sd_event_add_io.html#
     * 
     * @param revents - received events 
     * @param userdata - userdata supplied while calling sd_bus_attach_event() - always 'this'
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    static int incoming_mailbox_message_callback(
        sd_event_source *s, 
        int fd,
 	    uint32_t revents,
        void *userdata);
    /**
     * @brief handles incoming mailbox message sent by an external thread
     * 
     * For more information see :
     * https://www.freedesktop.org/software/systemd/man/sd_event_add_io.html#
     * 
     * @param s - the sd-event IO event source
     * @param fd - the Linux anonymous pipe file descriptor used to poll&read from the mailbox 
     * @param revents - received events 
     * @return int - 0 on success, On failure, return a negative Linux errno-style error code.
     */
    int incoming_mailbox_message_callback_impl(
        sd_event_source *s, 
        int fd, 
        uint32_t revents);  
        
    /*
    == D-Bus bus sub-module members==

    The bus sub-module perform lower layers sd-bus operations
    */

    /**
     * @brief initialize the bus sub-module
     * Can be called only when the object is in DBusAdapterState::UNINITALIZED state
     * @return MblError - return MblError - Error::None for success, therwise the failure reason 
     */
    MblError bus_init();

    /**
     * @brief deinitialize the bus sub-module
     * Can be called only when the object is in DBusAdapterState::INITALIZED state
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError bus_deinit();

    // sd-bus bus connection handle
    sd_bus                  *connection_handle_ = nullptr;

    // D-Bus service unique name automatically assigned by the D-Bus daemon after connecting 
    // to the bus
    const char              *unique_name_;

    // D-Bus service known name (the service name) acquired explicitly by request
    const char              *service_name_; 

     /*
    == sd-event members==

    The event sub-module perform lower layers sd-event operations
    */

    /**
     * @brief initialize the sd-event event loop object
     * Can be called only when the object is in DBusAdapterState::UNINITALIZED state
     * @return MblError - return MblError - Error::None for success, therwise the failure reason
     */
    MblError event_loop_init();

    /**
     * @brief deinitialize the sd-event event loop object
     * Can be called only when the object is in DBusAdapterState::INITALIZED state
     * @return MblError - Error::None for success, otherwise the failure reason
     */
    MblError event_loop_deinit();

    /**
     * @brief start running by entering the event loop
     * invokes sd_event_run() in a loop, thus implementing the actual event loop. The call returns 
     * as soon as exiting was requested using sd_event_exit(3). For more information see:
     * https://www.freedesktop.org/software/systemd/man/sd_event_run.html#
     * Can be called only when the object is in DBusAdapterState::INITALIZED state
     * 
     * @param stop_status - returned MblError status which should state the reason for stopping the 
     * event loop (the exit code specified when invoking sd_event_exit()). If the status is 
      * MblError::None then the exit is done gracefully.
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError event_loop_run(MblError &stop_status);

    /**
     * @brief Send event loop a request to stop. this call handles 2 cases 
     * 1) called by CCRB thread - self stop) only due to fatal errors.
     * 2) called by external thread - EXIT message is sent via mailbox. 
     * Can be called only when the object is in DBusAdapterState::RUNNING  state
     * 
     * For more information see:
     * https://www.freedesktop.org/software/systemd/man/sd_event_add_exit.html#
     * 
     * @param stop_status - sent MblError status which should state the reason for stopping the 
     * event loop (the exit code specified when invoking sd_event_exit()). If the status is 
      * MblError::None then the exit is done gracefully.
     * @return MblError - Error::None for success , otherwise the failure reason
     */
    MblError event_loop_request_stop(MblError stop_status);

    
    // sd-event event loop handle
    sd_event                *event_loop_handle_ = nullptr;

    /*
    == sd-bus message helper static calls==
    */
    
    /**
     * @brief converts sd-bus integer (enum) message type into a string
     * 
     * @param message_type - one of SD_BUS_MESSAGE_METHOD_CALL, SD_BUS_MESSAGE_METHOD_RETURN, 
     *  SD_BUS_MESSAGE_METHOD_ERROR, SD_BUS_MESSAGE_SIGNAL
     * @return const char* - string of one of the above
     */
    static const char* message_type_to_str(uint8_t message_type);

    /**
     * @brief validates sd-bus integer (enum) message type
     * 
     * @param message_type - one of SD_BUS_MESSAGE_METHOD_CALL, SD_BUS_MESSAGE_METHOD_RETURN, 
     *  SD_BUS_MESSAGE_METHOD_ERROR, SD_BUS_MESSAGE_SIGNAL
     * @return true if messsage type is valid
     * @return false if message type is invalid
     */
    static bool is_valid_message_type(uint8_t message_type);

    /* 
    A set which stores upper-layer-asynchronous bus request handles. the actual handle is 
    represented by the arriving message. the message should hold everything we need to know 
    about the current method call. The message stay 'alive' since its refernce is incremented.    
    */    
    // TODO - IMPORTANT - deallocate pending_messages_ on deinit - It is not done now since
    // any code I write will need to be fixed. Need to get a clearer view on how the system goes 
    // down
    std::set<const sd_bus_message*>    pending_messages_;

    /*
    The incoming mailbox is used as a one-way inter-thread lock-free way of communication. 
    The inner implementation is explain in the Mailbox class.
    The input (READ) edge of the mailbox is attached to the event loop as an IO event source 
    (sd_event_add_io). The event loop fires and event immediately after the message pointer is 
    written to the pipe.  
    */    
    Mailbox     mailbox_in_;   

    // An event manager to allow sending self defer events / self timer events
    // TODO - clear on deinit
    EventManager event_manager_;

    // The pthread_t thread ID of the initializing thread (CCRB thread) 
    pthread_t   initializer_thread_id_;

    /**
     * @brief This vector holds all pairs of match rules to add to bus connection
     * using sd_bus_add_match().
     * The 1st string is a short description
     * The 2nd string is the match rule
     */
    static const std::vector< std::pair<std::string, std::string> > match_rules;

    /**
     * @brief Add all match rules in match_rules vector. 
     * For now, I assume a slot for the source is unneeded. The callback is the same common 
     * callback for all arriving messages - incoming_bus_message_callback
    * 
    * @return MblError - DBA_SdBusRequestAddMatchFailed if failure , None for success.
    */
    MblError add_match_rules();

public:
    /*
    == API Implementation ==
    */

    /**
     * @brief Construct a new DBusAdapterImpl object  (2 step initialization)
     * 
     * @param ccrb - a reference to the Cloud Connect Resource Broker
     */
    DBusAdapterImpl(ResourceBroker &ccrb);
    
    /**
     * @brief initialize the object (2 step initialization)
     * Can be called only when the object is in DBusAdapterState::UNINITALIZED state
     * 
     * @return MblError - Error::None for success running the loop, otherwise the failure reason 
     */
    MblError init();

    /**
     * @brief deinitialize the object (2 step deinitialization)
     * Can be called only when the object is in DBusAdapterState::INITALIZED state
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError deinit();

    /**
     * @brief - start running
     * Can be called only when the object is in DBusAdapterState::INITALIZED state
     * @param stop_status returned MblError status which should state the reason for stopping the 
     * event loop (the exit code specified when invoking sd_event_exit()). If the status is 
      * MblError::None then the exit is done gracefully.
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError run(MblError &stop_status);

    /**
     * @brief 
     * Can be called only when the object is in DBusAdapterState::RUNNING state
     * @param stop_status - sent MblError status which should state the reason for stopping the 
     * event loop (the exit code specified when invoking sd_event_exit()). If the status is 
      * MblError::None then the exit is done gracefully.
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError stop(MblError stop_status);
    
    // TODO - IMPLEMENT
    /**
     * @brief   ===TBD===
     * 
     * @param ipc_request_handle 
     * @param reg_status 
     * @return MblError - Error::None for success running the loop, otherwise the failure reason
     */
    MblError handle_ccrb_RegisterResources_status_update(
        const uintptr_t ipc_request_handle, 
        const CloudConnectStatus reg_status);

    // TODO - IMPLEMENT
    /**
     * @brief  ===TBD===
     * 
     * @param ipc_request_handle 
     * @param dereg_status 
     * @return MblError 
     */
    MblError handle_ccrb_DeregisterResources_status_update(
        const uintptr_t ipc_request_handle, 
        const CloudConnectStatus dereg_status);

    // TODO - IMPLEMENT
    /**
     * @brief ===TBD===
     * 
     * @param ipc_request_handle 
     * @param add_status 
     * @return MblError 
     */
    MblError handle_ccrb_AddResourceInstances_status_update(
        const uintptr_t ipc_request_handle, 
        const CloudConnectStatus add_status);

    // TODO - IMPLEMENT
    /**
     * @brief  ===TBD===
     * 
     * @param ipc_request_handle 
     * @param remove_status 
     * @return MblError 
     */
    MblError handle_ccrb_RemoveResourceInstances_status_update(
        const uintptr_t ipc_request_handle, 
        const CloudConnectStatus remove_status);

    /**
 * @brief 
 * Send 'deferred event' to the event loop using 
 * sd_event_add_defer(). Must be called by CCRB thread only. 
 * For more details: https://www.freedesktop.org/software/systemd/man/sd_event_add_defer.html#
 * @param data - the data to be sent, must be formatted according to the data_type
 * @param data_length - length of data used, must be less than the maximum size allowed
 * @param data_type - the type of data to be sent.
 * @param callback - callback to be called when event is fired
 * @param out_event_id  - generated event id (for debug or to cancel the event)
 * @param description - optional - description for the event cause, can leave empty string or 
 * not supply at all
 * @return MblError - return MblError - Error::None for success, therwise the failure reason
 */
    MblError send_event_immediate(
        SelfEvent::EventData data,
        unsigned long data_length,
        SelfEvent::EventDataType data_type,        
        SelfEventCallback callback,
        uint64_t &out_event_id,
        const std::string description="");

    using DBusAdapterState = State::eState;
};


}  //namespace mbl {

#endif //_DBusAdapter_Impl_h_
