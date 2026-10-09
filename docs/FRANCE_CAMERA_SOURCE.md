# French camera source selection

The French source profile now selects four existing, independently verified
`SB/Core/x/iCamera.cpp` identities. No function bounds, registry records or
identity proofs change.

| Function | French address | Bytes | Registry |
| --- | --- | ---: | --- |
| iCameraUpdateFog | 0x35c930 | 1672 | relocation-corroborated-functions.json |
| iCameraSetNearFarClip | 0x35d230 | 64 | corroborated-functions.json |
| iCamGetViewMatrix | 0x35d270 | 156 | corroborated-functions.json |
| iCameraFrustumPlanes | 0x35d430 | 396 | corroborated-functions.json |

Each canonical linkage is independently recovered from the existing USA,
PAL and German original provenance; all three agree. With the PS2 camera
source recovery in `3902dfc9b`, the four-function French pilot is 100% exact,
adding 2,288 exact bytes and four exact functions. No extra call relocation
or identity is introduced by this selection. The source commit's full debug
region and GameCube controls remain documented in `PS2_CAMERA_SOURCE.md`.

Private evidence is `build/camera-selection-proof.json` (unchanged registry
records), `build/camera-selection-france-profile.json` and
`build/camera-selection-france-pilot/report.json`. The full production report
remains the integration gate. Other camera functions remain unselected until
their complete independent identities and source linkage are authenticated.
