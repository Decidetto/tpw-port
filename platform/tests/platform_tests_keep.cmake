# The functions of src/ the platform tests build (CMakeLists.txt, tpw_platform_tests): those of the files
# src/sound/ChannelState.cpp, src/sound/TbSoundCodecLoop.cpp, src/sound/TbSoundPipeChannel.cpp, src/sound/TbSoundPipe.cpp, src/sound/AudioComponent.cpp, src/sound/TbSoundSampleStore.cpp, src/sound/SystemCommand.cpp, src/sound/TbSoundCore.cpp, src/sound/TbSoundSampleWAV.cpp, src/sound/TbSoundSystemModule.cpp, src/game/online/email.cpp and
# src/email_load_Outbox_ReadMailData.cpp of before the regroup (v5L30b). They now share files with others, so the tests
# compile copies of these files (TPW_PLATFORM_TESTS_SOURCES, under src/) that keep only them (cmake/slicer.cmake).
set(TPW_PLATFORM_TESTS_SOURCES
  game/online/email.cpp
  sound/AudioComponent.cpp
  sound/ChannelState.cpp
  sound/ServiceQueue.cpp
  sound/SystemCommand.cpp
  sound/TbSoundCodecLoop.cpp
  sound/TbSoundCore.cpp
  sound/TbSoundPipe.cpp
  sound/TbSoundPipeChannel.cpp
  sound/TbSoundSampleStore.cpp
  sound/TbSoundSampleWAV.cpp
  sound/TbSoundSystemModule.cpp
)
set(TPW_PLATFORM_TESTS_KEEP
  __Z14SaveStreamCtorP10SaveStream   # SaveStreamCtor(SaveStream*)
  __Z14SaveStreamOpenP10SaveStreamPKc   # SaveStreamOpen(SaveStream*, char const*)
  __Z15SaveStreamCloseP10SaveStream   # SaveStreamClose(SaveStream*)
  __Z15SaveStreamThrowPKc   # SaveStreamThrow(char const*)
  __Z15SaveStreamWriteP10SaveStreamPKc   # SaveStreamWrite(SaveStream*, char const*)
  __Z17SaveStreamDestroyP10SaveStream   # SaveStreamDestroy(SaveStream*)
  __ZN11AddressBook4SaveEPKc   # AddressBook::Save(char const*)
  __ZN13outbox_callee10StreamCtorEP10SaveStream   # outbox_callee::StreamCtor(SaveStream*)
  __ZN13outbox_callee10StreamOpenEP10SaveStreamPKc   # outbox_callee::StreamOpen(SaveStream*, char const*)
  __ZN13outbox_callee11StreamWidenEP10SaveStream   # outbox_callee::StreamWiden(SaveStream*)
  __ZN13outbox_callee13StreamDestroyEP10SaveStream   # outbox_callee::StreamDestroy(SaveStream*)
  __ZN13outbox_callee13StreamGetlineEP10SaveStreamPcic   # outbox_callee::StreamGetline(SaveStream*, char*, int, char)
  __ZN6Outbox12ReadMailDataEi   # Outbox::ReadMailData(int)
  __ZN13CUniqueHandle8SetIndexEt   # CUniqueHandle::SetIndex(unsigned short)
  __ZN13CChannelState13mChannelStateE   # CChannelState::mChannelState
  __ZN13CChannelStateC1Ev   # CChannelState::CChannelState()
  __ZN13CUniqueHandle14_mBitsPerCountE   # CUniqueHandle::_mBitsPerCount
  __ZN9TbSpeaker10AutoDelete10_mpSpeakerE   # TbSpeaker::AutoDelete::_mpSpeaker
  __ZN9TbSpeaker10AutoDeleteD1Ev   # TbSpeaker::AutoDelete::~AutoDelete()
  __ZNK12CAtomicValuecviEv   # CAtomicValue::operator int() const
  __ZN19TbSoundSystemModuleD1Ev   # TbSoundSystemModule::~TbSoundSystemModule()
  __ZN12CAtomicValueC1Ev   # CAtomicValue::CAtomicValue()
  __ZN13CUniqueHandleC1EPv   # CUniqueHandle::CUniqueHandle(void*)
  __ZN13CUniqueHandleC1Ev   # CUniqueHandle::CUniqueHandle()
  __ZN20CAutoCriticalSectionC1Ev   # CAutoCriticalSection::CAutoCriticalSection()
  __ZN20CAutoCriticalSectionD1Ev   # CAutoCriticalSection::~CAutoCriticalSection()
  __ZN7CLowestclEjP18TbSoundPipeChannel   # CLowest::operator()(unsigned int, TbSoundPipeChannel*)
  __ZN13CChannelState24GetNumberOfChannelsInUseEv   # CChannelState::GetNumberOfChannelsInUse()
  __ZN19TbSoundSystemModule28SupportsEnvironmentalEffectsEv   # TbSoundSystemModule::SupportsEnvironmentalEffects()
  __ZN19TbSoundSystemModule7StopAllEv   # TbSoundSystemModule::StopAll()
  __ZN13CUniqueHandleppEv   # CUniqueHandle::operator++()
  __ZN20CAutoCriticalSectioncvPPvEv   # CAutoCriticalSection::operator void**()
  __ZN22TbSoundCriticalSectionC1EPv   # TbSoundCriticalSection::TbSoundCriticalSection(void*)
  __ZN22TbSoundCriticalSectionD1Ev   # TbSoundCriticalSection::~TbSoundCriticalSection()
  __ZN12CAtomicValuemmEv   # CAtomicValue::operator--()
  __ZN13CChannelState13RemoveChannelEv   # CChannelState::RemoveChannel()
  __ZN13CChannelState8InstanceEv   # CChannelState::Instance()
  __ZNK13CUniqueHandle8GetIndexEv   # CUniqueHandle::GetIndex() const
  __ZNK13CUniqueHandlecvPvEv   # CUniqueHandle::operator void*() const
  __ZN13CChannelState10AddChannelEv   # CChannelState::AddChannel()
  __ZN7CLowestC1Ev   # CLowest::CLowest()
  __ZN12CAtomicValueppEv   # CAtomicValue::operator++()
  __ZN19TbSoundSystemModule10RestartAllEv   # TbSoundSystemModule::RestartAll()
  __ZN19TbSoundSystemModule10SetOptionsEj   # TbSoundSystemModule::SetOptions(unsigned int)
  __ZN19TbSoundSystemModule10SetPanRateEi   # TbSoundSystemModule::SetPanRate(int)
  __ZN19TbSoundSystemModule11GetPositionEjR15TbSoundPosition   # TbSoundSystemModule::GetPosition(unsigned int, TbSoundPosition&)
  __ZN19TbSoundSystemModule11GetVelocityEjR15TbSoundPosition   # TbSoundSystemModule::GetVelocity(unsigned int, TbSoundPosition&)
  __ZN19TbSoundSystemModule11SetPositionEjRK15TbSoundPositioni   # TbSoundSystemModule::SetPosition(unsigned int, TbSoundPosition const&, int)
  __ZN19TbSoundSystemModule11SetVelocityEjRK15TbSoundPosition   # TbSoundSystemModule::SetVelocity(unsigned int, TbSoundPosition const&)
  __ZN19TbSoundSystemModule12PauseChannelEj   # TbSoundSystemModule::PauseChannel(unsigned int)
  __ZN19TbSoundSystemModule12SetFrequencyEjjf   # TbSoundSystemModule::SetFrequency(unsigned int, unsigned int, float)
  __ZN19TbSoundSystemModule13GetBufferSizeEP18TbSoundPipeChannel   # TbSoundSystemModule::GetBufferSize(TbSoundPipeChannel*)
  __ZN19TbSoundSystemModule14RestartChannelEj   # TbSoundSystemModule::RestartChannel(unsigned int)
  __ZN19TbSoundSystemModule14SetChannelInfoEjP18TbSoundPipeChannelR17TbSoundSampleInfo   # TbSoundSystemModule::SetChannelInfo(unsigned int, TbSoundPipeChannel*, TbSoundSampleInfo&)
  __ZN19TbSoundSystemModule15GetSystemObjectEj   # TbSoundSystemModule::GetSystemObject(unsigned int)
  __ZN19TbSoundSystemModule15SetSpeedOfSoundEf   # TbSoundSystemModule::SetSpeedOfSound(float)
  __ZN19TbSoundSystemModule16IsChannelPlayingEj   # TbSoundSystemModule::IsChannelPlaying(unsigned int)
  __ZN19TbSoundSystemModule16SetSpeakerConfigERKN9TbSpeaker6ConfigE   # TbSoundSystemModule::SetSpeakerConfig(TbSpeaker::Config const&)
  __ZN19TbSoundSystemModule17SetDistanceFactorEjRK15TbSoundDistance   # TbSoundSystemModule::SetDistanceFactor(unsigned int, TbSoundDistance const&)
  __ZN19TbSoundSystemModule17SetReverbPositionERK15TbSoundPosition   # TbSoundSystemModule::SetReverbPosition(TbSoundPosition const&)
  __ZN19TbSoundSystemModule19SetListenerPositionERK15TbSoundPosition   # TbSoundSystemModule::SetListenerPosition(TbSoundPosition const&)
  __ZN19TbSoundSystemModule19SetListenerVelocityERK15TbSoundPosition   # TbSoundSystemModule::SetListenerVelocity(TbSoundPosition const&)
  __ZN19TbSoundSystemModule20GetDirectSoundBufferEjRP18IDirectSoundBuffer   # TbSoundSystemModule::GetDirectSoundBuffer(unsigned int, IDirectSoundBuffer*&)
  __ZN19TbSoundSystemModule20GetDirectSoundObjectERP12IDirectSound   # TbSoundSystemModule::GetDirectSoundObject(IDirectSound*&)
  __ZN19TbSoundSystemModule20SetListenerDirectionERK15TbSoundPositionS2_   # TbSoundSystemModule::SetListenerDirection(TbSoundPosition const&, TbSoundPosition const&)
  __ZN19TbSoundSystemModule20SetReverbEnvironmentEjfff   # TbSoundSystemModule::SetReverbEnvironment(unsigned int, float, float, float)
  __ZN19TbSoundSystemModule21Set3DDSPFunctionalityEjj   # TbSoundSystemModule::Set3DDSPFunctionality(unsigned int, unsigned int)
  __ZN19TbSoundSystemModule28ConvertPitchIndexToFrequencyEi   # TbSoundSystemModule::ConvertPitchIndexToFrequency(int)
  __ZN19TbSoundSystemModule4InitERK17TbSoundSystemInfo   # TbSoundSystemModule::Init(TbSoundSystemInfo const&)
  __ZN19TbSoundSystemModule7ProcessEv   # TbSoundSystemModule::Process()
  __ZN19TbSoundSystemModule8PauseAllEv   # TbSoundSystemModule::PauseAll()
  __ZN19TbSoundSystemModule8SetPitchEjji   # TbSoundSystemModule::SetPitch(unsigned int, unsigned int, int)
  __ZN19TbSoundSystemModule9SetLoPassEjf   # TbSoundSystemModule::SetLoPass(unsigned int, float)
  __ZN19TbSoundSystemModuleC1Ev   # TbSoundSystemModule::TbSoundSystemModule()
)
