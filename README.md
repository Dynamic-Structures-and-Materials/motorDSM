motorDSM
==========

EPICS motor drivers for the following [Dynamic Structures and Materials](https://www.dynamic-structures.com/) motor controllers: MD-90

motorDSM is a submodule of [motor](https://github.com/epics-modules/motor).  When motorDSM is built in the ``motor/modules`` directory, no manual configuration is needed.

motorDSM can also be built outside of motor by copying it's ``configure/EXAMPLE_RELEASE.local`` file to ``RELEASE.local`` and defining the paths to ``EPICS_BASE``, ``MOTOR``, and itself.

motorDSM contains an example IOC that is built if ``configure/CONFIG_SITE.local`` sets ``BUILD_IOCS = YES``.  The example IOC can be built outside of the driver module.  Copy ``iocs/dsmIOC/configure/EXAMPLE_RELEASE.local`` to ``RELEASE.local`` and uncomment and set the paths for the appropriate lines depending on whether motorDSM was built inside the motor module or independently.


-------------------------------------------------
Compiling motorDSM
-------------------------------------------------

To set up a full EPICS stack for development and testing, install and configure all of the following dependencies:

------------------------
### epics-base

Install make, gcc, and perl packages if not already installed, then clone and build epics-base:

    $ export SUPPORT=/path/to/install/directory
    $ cd $SUPPORT
    $ git clone git@github.com:epics-base/epics-base.git
    $ cd epics-base
    $ make distclean
    $ make


------------------------
### asyn

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
### seq

    $ cd $SUPPORT
    $ git clone git@github.com:ISISComputingGroup/EPICS-seq.git seq

Install the ``re2c`` package (Arch) if needed.

Create ``seq/configure/RELEASE.local`` and set path for ``EPICS_BASE``.  (Note this package seems to forget to git-ignore the .local file.)

Edit ``seq/configure/RELEASE`` to add the missing '-' before the ``include`` for ``ISIS_CONFIG`` on the next to last line.  This seems to be a typo.

    $ cd $SUPPORT/seq
    $ make clean
    $ make


------------------------
### motor

    $ cd $SUPPORT
    $ git clone git@github.com:epics-modules/motor.git

Create ``motor/configure/RELEASE.local`` and set ``SUPPORT``, ``ASYN``, ``SNCSEQ``, and ``EPICS_BASE`` to the appropriate paths.

    $ cd $SUPPORT/motor
    $ make distclean
    $ make


------------------------
### motorDSM (this package)

    $ cd $SUPPORT
    $ git clone git@github.com:Binary-Coalescence/motorDSM.git

In ``motorDSM/configure``, copy ``EXAMPLE_RELEASE.local`` to ``RELEASE.local`` and set paths for ``EPICS_BASE``, ``MOTOR``, and ``MOTOR_DSM``.

In ``motorDSM/configure``, copy ``EXAMPLE_CONFIG_SITE.local`` to ``CONFIG_SITE.local`` and uncomment to set:  
	BUILD_IOCS = YES

In ``motorDSM/iocs/dsmIOC/configure``, copy ``EXAMPLE_RELEASE.local`` to ``RELEASE.local``.  Comment out the "if built inside motor" lines, uncomment the "if built outside motor" lines, and set the path for ``MOTOR_DSM``.

    $ cd $SUPPORT/motorDSM
    $ make distclean
    $ make


-------------------------------------------------
Configuring the IOC server
-------------------------------------------------

The directory `$SUPPORT/motorDSM/iocs/dsmIOC/iocBoot/iocDSM` contains example configurations for the IOC server that runs on the computer the motor controllers are attached to.  The `st.cmd.md90` and `motor.substitutions.md90` files provide an example to configure and run one attached MD-90.  The `st.cmd.md90.multi` and `motor.substitutions.md90.multi` files provide an example to configure and run eight attached MD-90s connected on ports `dev/ttyUSB0` through `/dev/ttyUSB7`.  Add or remove lines from the `*.multi` files as necessary to configure a different number of attached MD-90s.

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


-------------------------------------------------
Running the example IOC
-------------------------------------------------

To run the example IOC configured above:

1. Follow the steps in "Configuring the system for attached controllers" below.

3. In the ``iocs/dsmIOC/iocBoot/iocDsm`` directory, run  
`$ ../../bin/linux-x86_64/dsm st.cmd.md90`  
for one attached MD-90 controller, or  
`$ ../../bin/linux-x86_64/dsm st.cmd.md90.multi`  
for multiple attached MD-90 controllers.  This is set up for eight controllers, so add or remove lines as appropriate if using a different number.

4. Test using the `caget` and `caput` programs from the `epics-base` package as described in the "Example usage" section below.


-------------------------------------------------
Example usage
-------------------------------------------------

1. After building, run the example IOC described above in one terminal window.

2. Open another terminal window on either the same computer as the IOC or another computer on the LAN and navigate to `[EPICS install directory]/epics-base/bin/linux-x86_64/` (or wherever you built EPICS base).  You could alternatively add this directory to your PATH.

3. Set the "EPICS_CA_ADDR_LIST" environment variable to include the IP address of the server.  If it's running on the same computer, you can use the loopback IP address:  
`$ export EPICS_CA_ADDR_LIST='127.0.0.1'`  

4.  Use the `caget` and `caput` programs to read and set process variables, respectively.  

For example, to get the current position (in encoder counts of 10 nm), use:  
`$ ./caget DSM:m0.REP`  
This reads the REP variable, which is the "Raw Encoder Position".  Additionally, change `m0` to `m1`, `m2`, etc. to read the values from other motors when running more than one.

Homing the motor (must be done before you can issue position commands):  
`$ ./caput DSM:m0.HOMF 1`  
This starts the homing sequence in the forward direction.  Alternatively, for homing starting in the reverse direction:  
`$ ./caput DSM:m0.HOMR 1`

Moving to a position target:  
`$ ./caput DSM:m0.VAL 2.345`  
This moves the motor to 2.345 mm.

Setting a velocity target:  
`$ ./caput DSM:m0.VELO 0.5`  
This sets the velocity target to 0.5 mm/s.  (Note that velocity targets are approximate only.  They adjust the step rate of the motor and are not guaranteed to be exact.)


-------------------------------------------------
A note about velocity targets
-------------------------------------------------

The I-20 motor driven by the MD-90 is a closed loop "step and repeat" motor that takes full steps towards its position target until it is close, then will perform linear extensions to close the loop on the target position.  This is handled internally on the MD-90, not by EPICS.

The velocity target parameter sets the step frequency at which the motor operates on its way to the target position.  The speed is not closed loop, and will depend on external loads, environmental conditions, etc.  A speed target of 1 mm/s will generate a roughly 1 mm/s motion, but it is not guaranteed.

Additionally, due to the way EPICS operates, setting `VELO` will not immediately send a command to the MD-90.  Instead, EPICS remembers the last value you set, and will set this new velocity target when it sends the next move command.  **However, the motor must not be in servo mode to accept a new velocity target.**

The motor enters servo mode when you send a new position target, and stays in servo mode until you issue a Stop command (by setting the `DSM:m0.STOP` parameter to 1).

If you do not disable servo prior to issuing a Move command at the new velocity, then `VELO` will become out of sync with the actual motor velocity, and EPICS will return error 3 "Cannot execute while moving" in its console each time you issue a Move command.  This is because each Move command internally sends a "Set step frequency" command, which will error if you do not Stop the motor first.  Reading the VELO parameter at this point will return the wrong value--it returns the value you requested, not the actual speed setting on the motor.  To fix this, you must Stop the motor, then send a new Move command.

