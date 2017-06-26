clearwater-snmp-handlers
========================

Overview
--------

This repository holds the code for SNMP extension modules to provide stats and alarms for Clearwater nodes.

Packages
--------

It contains the following packages:

* clearwater-snmp-alarm-agent: alarms for Clearwater nodes
* clearwater-snmp-handler-cdiv: stats for Call Diversion AS nodes
* clearwater-snmp-handler-memento: stats for Memento (HTTP) nodes
* clearwater-snmp-handler-memento-as: stats for Memento (SIP) AS nodes

Building Project Clearwater MIB
-------------------------------

The MIB for Project Clearwater is auto-generated from MIB fragments in `mib-generator`. To build the MIB, run `mib-generator/cw_mib_generator.py` using Python.
