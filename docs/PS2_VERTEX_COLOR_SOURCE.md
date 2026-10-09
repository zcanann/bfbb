# PS2 immediate-mode vertex color setter

The pinned [RenderWare 3.5 PS2 SDK header](https://github.com/sigmaco/rwsdk-v3.5-ps2/blob/82a98fe97a6346388e27e0c73068fc326109de68/rwsdk/include/sky2/rwcore.h#L794)
implements `RwIm3DVertexSetRGBA` by taking a constant pointer to the vertex's color
and assigning its four channels. The reconstructed PS2 header instead constructed
a temporary color and then copied it into the vertex. The authentic setter form
restores the original color-pointer lifetime and substantially improves several
renderers. The existing `preLitColor` member spelling and all aggregate layouts
remain unchanged.

The referenced SDK file is pinned to commit
`82a98fe97a6346388e27e0c73068fc326109de68`; its SHA-256 is
`a65c6d4c54676cf1314e9d77ea0ef5c02454047e020e0271d067df9a277f3b3f`.
The private copy is `build/rwcore-sdk-82a98fe.h`. No compiler patch, comparison rule,
function identity or denominator is changed.

The direct include/call audit found thirteen profiled translation units using the
macro. Following the inline `xLaserBolt.h` include chain adds eight consumers,
making twenty-one profiled translation units in each debug region. Every macro
invocation uses a normal vertex pointer or array-index expression; none has a
side-effecting vertex argument. The same pointer and all four channel expressions
are each evaluated once.

The first complete US sweep exposed one regression in the cinematic laser callback.
The laser helper now constructs its white color, including the requested alpha,
with four explicit channel assignments and copies it through the SDK's color-copy setter. This
preserves the prior laser code while allowing the ordinary channel setter to use
its authentic SDK implementation. This color aggregate is selected only on PS2:
the shared form changed the already exact GameCube helper. With the platform guard,
complete GU4Y78 laser and cinematic comparisons preserve every function score,
including 50 exact functions / 6496 bytes and 92 / 17140, respectively.

An aggregate initializer also restores the cinematic callback but regresses the
standalone laser renderer. Assigning the four fields before copying preserves
both callers and their original generated code. The final helper was recompiled
in all eight header consumers, including the two available French consumers.

Complete comparisons cover 21 units each for US, Europe and Germany, and all six
identified French consumers. Each debug region gains the exact 1724-byte
`xFXShineRender`; eight other renderer functions improve, with no function-score
regressions. The French consumer scores remain unchanged.

| Function | Before | After |
| --- | ---: | ---: |
| `iRenderPushFlat` | 88.80236% | 94.840706% |
| `iRenderPushQuadStreak` | 84.63438% | 90.90938% |
| `xFXShineRender` | 71.17633% | 100% |
| `xFXStreakRender` | 59.995743% | 98.76596% |
| `xScrFXGlareRender` | 76.66723% | 97.3429% |
| `RenderLightning` | 81.98026% | 99.83365% |
| `zLightningFunc_Render` | 92.142265% | 95.46598% |
| `SpringRender` | 91.746376% | 95.14855% |
| `RendConeOfDeath` | 95.25915% | 99.05488% |

The source comparison snapshot is `bb681d9d3`. Private evidence includes the
recursive consumer audit (`build/rgba-consumers.py` and `.json`), initial complete
US comparison, final regional comparison (`build/rgba-verified-comparison.json`),
function deltas (`build/rgba-verified-changes.json`), and the two GameCube helper comparisons.
These remain ordinary complete-unit source comparisons against the independently
identified original members. Existing unresolved RenderWare calls and data
references retain their normal limitations; a code score is not a full-link claim.
