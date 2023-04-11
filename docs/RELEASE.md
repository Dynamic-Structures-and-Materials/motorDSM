# motorAcs Releases

## __R1-1-1 (2023-04-11)__
R1-1-1 is a release based on the master branch.  

### Changes since R1-1

#### New features
* None

#### Modifications to existing features
* None

#### Bug fixes
* None

#### Continuous integration
* Added ci-scripts (v3.0.1)
* Switched from Travis CI to Github Actions

## __R1-1 (2020-05-12)__
R1-1 is a release based on the master branch.  

### Changes since R1-0

#### New features
* None

#### Modifications to existing features
* Commit [1bd580a](https://github.com/epics-motor/motorAcs/commit/1bd580a87869fb140939978c0b06856917282da9): ``iocsh`` dir moved into ``acsApp`` dir; ``ACS_MCB4B.iocsh`` is now installed at build time 

#### Bug fixes
* Commit [5ab502c](https://github.com/epics-motor/motorAcs/commit/5ab502c53ac81885e2a511ade95f22d0a0db4f43): Include ``$(MOTOR)/modules/RELEASE.$(EPICS_HOST_ARCH).local`` instead of ``$(MOTOR)/configure/RELEASE``
* Pull request [#1](https://github.com/epics-motor/motorAcs/pull/1): Eliminated compiler warnings

## __R1-0 (2019-04-18)__
R1-0 is a release based on the master branch.  

### Changes since motor-6-11

motorAcs is now a standalone module, as well as a submodule of [motor](https://github.com/epics-modules/motor)

#### New features
* motorAcs can be built outside of the motor directory
* motorAcs has a dedicated example IOC that can be built outside of motorAcs

#### Modifications to existing features
* None

#### Bug fixes
* None
