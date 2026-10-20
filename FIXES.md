# The port's fixes

Each entry is a place where tpw-port deliberately behaves differently from the 1999 Mac build. tpw-decomp keeps the
Mac's behaviour, bugs included; the port builds the file under overrides/ in place of the submodule's file of the
same path. Everything else in the port is the platform layer (platform/, launcher/), which replaces the Mac's
system calls and is not a change to the game's behaviour.

| File | Function | The Mac | The port | Why | Unit |
| --- | --- | --- | --- | --- | --- |
| overrides/src/RideDestroy_CalculateSkirtVertices.cpp | CalculateSkirtVertices | a 256-entry list buffer, so the incagod's 275 entries run on into a dead local | 1024 entries | the host's stack guard sits past the buffer (SIGABRT loading a Full Simulation park) | v5L13 |
