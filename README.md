motorDSM
==========

EPICS motor drivers for the following [Dynamic Structures and Materials](https://www.dynamic-structures.com/) motor controllers: MD-90

motorDSM is a submodule of [motor](https://github.com/epics-modules/motor).  When motorDSM is built in the ``motor/modules`` directory, no manual configuration is needed.

motorDSM can also be built outside of motor by copying it's ``configure/EXAMPLE_RELEASE.local`` file to ``RELEASE.local`` and defining the paths to ``EPICS_BASE``, ``MOTOR``, and itself.

motorDSM contains an example IOC that is built if ``configure/CONFIG_SITE.local`` sets ``BUILD_IOCS = YES``.  The example IOC can be built outside of the driver module.  Copy ``iocs/dsmIOC/configure/EXAMPLE_RELEASE.local`` to ``RELEASE.local`` and uncomment and set the paths for the appropriate lines depending on whether motorDSM was built inside the motor module or independently.

# Running an example IOC

To run the example IOC, build the packages listed below, then:  

1. Follow the steps in "Configuring the system for attached controllers" below.
2. Set the "EPICS_CA_ADDR_LIST" environment variable to include the IP address of the server.
If it's running on the same computer, you can use the loopback IP address.  
`export EPICS_CA_ADDR_LIST='127.0.0.1`  
3. In the ``iocs/dsmIOC/iocBoot/iocDsm`` directory, run  
```
$ ../../bin/linux-x86_64/dsm st.cmd.md90
```

for one attached MD-90 controller, or
```
$ ../../bin/linux-x86_64/dsm st.cmd.md90.multi
```
for eight attached MD-90 controllers. Edit this file to use more than one unit; simply comment out the ones you don't need.  

4. Test using the `caget` and `caput` arguments as described in the "Example usage" section below.

# Configuring the system for attached controllers
-------------------------------------------------

The following steps must be used in either st.cmd.md90 (for a single unit) or in st.cmd.md90.multiple (for multiple units).
st.cmd.md90.multiple includes 8 motors predefined on /dev/ttyUSB0 through /dev/ttyUSB7. Comment out all motors you don't need.  

**1. Define a new serial port named "serial0" and set the location of the physical port**  
`drvAsynSerialPortConfigure([serial name], [device location], 0, 0, 0)`  
*e.g., `drvAsynSerialPortConfigure("serial0", "/dev/ttyUSB0", 0, 0, 0)`*  

**2. Configure the port**  
- Baud = 115200
- Bits = 8
- Parity = none
- Stop bits = 1
- Input end of message: "\r"
- Output end of message: "\r"
- Trace IO mask: 2
```
asynSetOption([serial name], 0, "baud", "115200")
asynSetOption([serial name], 0, "bits", "8")
asynSetOption([serial name], 0, "parity", "none")
asynSetOption([serial name], 0, "stop", "1")
asynOctetSetInputEos("serial0", 0, "\r")
asynOctetSetOutputEos("serial0", 0, "\r")
asynSetTraceIOMask("serial0", 0, 2)
```
where `[serial name]` is the name you assigned in step 1, surrounded by double quotes.

**3. Set initial parameters**  
- Power supply enabled (`EPS` command)
- Deadband = 10 nm (`SDB 10` command)  
```
asynOctetConnect("initConnection", [serial name], 0)
asynOctetWrite("initConnection", "EPS")
asynOctetWrite("initConnection", "SDB 10")
asynOctetDisconnect('initConnection')
```

**4. Create MD90 Controller object**  
`MD90CreateController([controller name], [serial name], 1, 100, 5000)`  
where `[controller name]` is the name of the motor to assign. Convention is to use "MD90n", starting with n=0.

**5. Intialize the IOC**  
After the call to `iocInit` (still in the st.cmd.md90[.multiple] file), issue the following commands for each motor. The example below uses `DSM:m0` but it should be run for each line described in motor.substitutions.md90 (or motor.substitutions.md90.multiple).
````
dbpf("DSM:m0.RTRY", "0")	#sets retries to 0; this is automatic on the MD90
dbpf("DSM:m0.TWV", "0.1")	#Tweak distance
dbpf("DSM:m0.VMAX", "1.0")	#Sets max velocity to 1 mm/s
dbpf("DSM:m0.HVEL", "1.0")	#Sets max velocity to 1 mm/s
````

**6. Update the substitutions file**  
Save and close the st.cmd file you've been configuring, then open the motor substitutions file (motor.substitutions.md90[.multiple]).
Ensure the values in the `pattern` block's `PORT` field match the names used in the std.cmd file.
Note that, despite this field being called "Port", they use the names of the MD90 Controller object defined above (by default, MD900, MD901, etc.
Do __not__ use the direct serial port names (by default, serial0, serial1, etc.).  


# Compiling motorDSM

------------------------

To set up a full EPICS stack for development and testing, install and configure all of the following dependencies:

------------------------
epics-base
------------------------

Install make, gcc, and perl packages if not already installed, then clone and build epics-base:

    $ export SUPPORT=/path/to/install/directory
    $ cd $SUPPORT
    $ git clone git@github.com:epics-base/epics-base.git
    $ cd epics-base
    $ make distclean
    $ make


------------------------
asyn
------------------------

    $ cd $SUPPORT
    $ git clone git@github.com:epics-modules/asyn.git

You may need to install (on Arch Linux) ``rpcsvc-proto`` package to get ``rpcgen`` binary needed to make asyn.

In ``asyn/configure``, create the file ``RELEASE.local`` with contents:  
SUPPORT=/path/to/install/directory  
EPICS_BASE=/path/to/epics-base

In ``asyn/configure``, create ``CONFIG_SITE.local`` file with the line:  
	TIRPC=YES  
if appropriate header files are in ``/usr/include/tirpc/rpc`` instead of ``/usr/include/rpc``.

    $ cd $SUPPORT/asyn
    $ make clean
    $ make


------------------------
seq
------------------------

    $ cd $SUPPORT
    $ git clone git@github.com:ISISComputingGroup/EPICS-seq.git seq

Install the ``re2c`` package (Arch) if needed.

Create ``seq/configure/RELEASE.local`` and set path for ``EPICS_BASE``.  (Note this package seems to forget to git-ignore the .local file.)

Edit ``seq/configure/RELEASE`` to add the missing '-' before the ``include`` for ``ISIS_CONFIG`` on the next to last line.  This seems to be a typo.

    $ cd $SUPPORT/seq
    $ make clean
    $ make


------------------------
motor
------------------------

    $ cd $SUPPORT
    $ git clone git@github.com:epics-modules/motor.git

Create ``motor/configure/RELEASE.local`` and set ``SUPPORT``, ``ASYN``, ``SNCSEQ``, and ``EPICS_BASE`` to the appropriate paths.

    $ cd $SUPPORT/motor
    $ make distclean
    $ make


------------------------
motorDSM (this package)
------------------------

    $ cd $SUPPORT
    $ git clone git@github.com:Binary-Coalescence/motorDSM.git

In ``motorDSM/configure``, copy ``EXAMPLE_RELEASE.local`` to ``RELEASE.local`` and set paths for ``EPICS_BASE``, ``MOTOR``, and ``MOTOR_DSM``.

In ``motorDSM/configure``, copy ``EXAMPLE_CONFIG_SITE.local`` to ``CONFIG_SITE.local`` and uncomment to set:  
	BUILD_IOCS = YES

In ``motorDSM/iocs/dsmIOC/configure``, copy ``EXAMPLE_RELEASE.local`` to ``RELEASE.local``.  Comment out the "if built inside motor" lines, uncomment the "if built outside motor" lines, and set the path for ``MOTOR_DSM``.

    $ cd $SUPPORT/motorDSM
    $ make distclean
    $ make

------------------------
# Example usage

After building, run the example IOC described at the top of this section in one terminal window.
Open another terminal window and navigate to [EPICS install directory]/epics-base/bin/linux-x86_64/ (or wherever you built EPICS base.  
Use the commands `caput` and `caget` to set and read process variables.  

For example, to get the current position, use `./caget DSM:m0.REP`. This reads the REP variable, which is the "Raw Encoder Position". Set m0 to m1, m2, etc. for multiple motors.

Other examples
-------------------------

Homing the motor (must be done before you can issue position commands):  
`./caput DSM:m0.HOMF 1			#Begins homing sequence in the forward direction`  
`./caput DSM:m0.HOMR 1			#Begins homing sequence in the reverse direction`  

Moving to a position target:  
`./caput DSM:m0.VAL 2.345		#Moves to 2.345 mm`

Setting a velocity target:  
`./caput DSM:m0.VELO 0.5		#Sets velocity target to 0.5 mm/s`  
(Note that velocity targets are appropriate only. They adjust the step rate of the motor and are not guaranteed to be exact.)