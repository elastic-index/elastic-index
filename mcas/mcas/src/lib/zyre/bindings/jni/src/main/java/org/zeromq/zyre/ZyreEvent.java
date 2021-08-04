/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
package org.zeromq.zyre;
import org.zeromq.czmq.*;

public class ZyreEvent implements AutoCloseable{
    static {
        try {
            System.loadLibrary ("zyrejni");
        }
        catch (Exception e) {
            System.exit (-1);
        }
    }
    public long self;
    /*
    Constructor: receive an event from the zyre node, wraps zyre_recv.
    The event may be a control message (ENTER, EXIT, JOIN, LEAVE) or  
    data (WHISPER, SHOUT).                                            
    */
    native static long __new (long node);
    public ZyreEvent (Zyre node) {
        /*  TODO: if __new fails, self is null...            */
        self = __new (node.self);
    }
    public ZyreEvent (long pointer) {
        self = pointer;
    }
    /*
    Destructor; destroys an event instance
    */
    native static void __destroy (long self);
    @Override
    public void close () {
        __destroy (self);
        self = 0;
    }
    /*
    Returns event type, as printable uppercase string. Choices are:   
    "ENTER", "EXIT", "JOIN", "LEAVE", "EVASIVE", "WHISPER" and "SHOUT"
    and for the local node: "STOP"                                    
    */
    native static String __type (long self);
    public String type () {
        return __type (self);
    }
    /*
    Return the sending peer's uuid as a string
    */
    native static String __peerUuid (long self);
    public String peerUuid () {
        return __peerUuid (self);
    }
    /*
    Return the sending peer's public name as a string
    */
    native static String __peerName (long self);
    public String peerName () {
        return __peerName (self);
    }
    /*
    Return the sending peer's ipaddress as a string
    */
    native static String __peerAddr (long self);
    public String peerAddr () {
        return __peerAddr (self);
    }
    /*
    Returns the event headers, or NULL if there are none
    */
    native static long __headers (long self);
    public Zhash headers () {
        return new Zhash (__headers (self));
    }
    /*
    Returns value of a header from the message headers   
    obtained by ENTER. Return NULL if no value was found.
    */
    native static String __header (long self, String name);
    public String header (String name) {
        return __header (self, name);
    }
    /*
    Returns the group name that a SHOUT event was sent to
    */
    native static String __group (long self);
    public String group () {
        return __group (self);
    }
    /*
    Returns the incoming message payload; the caller can modify the
    message but does not own it and should not destroy it.         
    */
    native static long __msg (long self);
    public Zmsg msg () {
        return new Zmsg (__msg (self));
    }
    /*
    Returns the incoming message payload, and pass ownership to the   
    caller. The caller must destroy the message when finished with it.
    After called on the given event, further calls will return NULL.  
    */
    native static long __getMsg (long self);
    public Zmsg getMsg () {
        return new Zmsg (__getMsg (self));
    }
    /*
    Print event to zsys log
    */
    native static void __print (long self);
    public void print () {
        __print (self);
    }
    /*
    Self test of this class.
    */
    native static void __test (boolean verbose);
    public static void test (boolean verbose) {
        __test (verbose);
    }
}