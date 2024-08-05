motorDSM
==========

EPICS motor drivers for the following [Dynamic Structures and Materials](https://www.dynamic-structures.com/) motor controllers: MD-90

motorDSM is a submodule of [motor](https://github.com/epics-modules/motor).  When motorDSM is built in the ``motor/modules`` directory, no manual configuration is needed.

motorDSM can also be built outside of motor by copying it's ``configure/EXAMPLE_RELEASE.local`` file to ``RELEASE.local`` and defining the paths to ``EPICS_BASE``, ``MOTOR``, and itself.

motorDSM contains an example IOC that is built if ``configure/CONFIG_SITE.local`` sets ``BUILD_IOCS = YES``.  The example IOC can be built outside of the driver module.  Copy ``iocs/dsmIOC/configure/EXAMPLE_RELEASE.local`` to ``RELEASE.local`` and uncomment and set the paths for the appropriate lines depending on whether motorDSM was built inside the motor module or independently.

To run the example IOC, in the ``iocs/dsmIOC/iocBoot/iocDsm`` directory, run

    $ ../../bin/linux-x86_64/dsm st.cmd.md90

for one attached MD-90 controller, or

    $ ../../bin/linux-x86_64/dsm st.cmd.md90.multi

for eight attached MD-90 controllers.  You may need to change the path(s) for the serial port(s) in ``st.cmd.md90`` or ``st.cmd.md90.multi`` if the MD-90 is not attached at ``/dev/ttyUSB0``.

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
