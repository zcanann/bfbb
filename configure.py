#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.append(str(Path(__file__).parent / "tools"))
import aliaspatch_link  # noqa: E402  -- for the AliasPatch.c dependency path

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GQPE78",  # 0
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-1"
config.compilers_tag = "20250812"
config.dtk_tag = "v1.7.0"
config.objdiff_tag = "v3.7.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.0-beta.5"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
    config.ldflags.append("-sym full")
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = 65  # Battle for Bikini Bottom

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-W err",
    # "-W all",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    "-i src/PowerPC_EABI_Support/include",
    "-i src/dolphin/include",
    "-i src/dolphin/src",
    "-i src/bink/include",
    "-i src/bink/src",
    "-i src",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym full", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

    # Warning flags
if args.warn == "all":
    cflags_base.append("-W all")
elif args.warn == "off":
    cflags_base.append("-W off")
elif args.warn == "error":
    cflags_base.append("-W error")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

cflags_msl_gc13_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-common off",
    "-inline deferred,auto",
    "-char signed",
    "-lang=c",
]

cflags_msl_runtime_c = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-common off",
    "-inline deferred,auto",
    "-fp_contract off",
    "-char signed",
    "-lang=c",
]

cflags_msl_runtime_cpp = [
    *cflags_runtime,
    "-lang=c++",
]

# dolphin library flags
cflags_dolphin = [
    *cflags_base,
    "-lang=c",
    "-fp fmadd",
    "-fp_contract off",
    "-char signed",
    "-str reuse",
    "-common off",
    "-O4,p",
    #"-requireprotos"
]

cflags_trk = [
    *cflags_base,
    "-O4,p",
            "-sdata 0",
            "-sdata2 0",
            "-inline auto,deferred",
            "-rostr",
            "-char signed",
            "-use_lmw_stmw on"
]

# Bink was compiled with ProDG
cflags_bink = [
    "-O2",
    "-mcpu=750",
    "-fno-exceptions",
    "-Wno-inline",
    "-nostdinc",
    "-I src/dolphin/src",
    "-I include",
    "-I src/dolphin/include",
    "-D__GEKKO__",
    "-I src/bink/include",
    "-I src/PowerPC_EABI_Support/include",
    "-G8",
]

# Renderware library flags
cflags_renderware = [
    *cflags_base,
    "-lang=c",
    "-fp fmadd",
    "-fp_contract on",
    "-char signed",
    "-str reuse",
    "-common off",
    "-O4,p",
    #"-requireprotos"
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-sdata 0",
    "-sdata2 0",
]

# Game-specific flags
cflags_bfbb = [
    *cflags_base,
    "-lang=c++",
    "-common on",
    "-char unsigned",
    "-str reuse,pool,readonly",
    "-use_lmw_stmw on",
    '-pragma "cpp_extensions on"',
    "-inline off",
    "-gccinc",
    "-i include/inline",
    "-i include/rwsdk",
    "-i src/SB/Core/gc",
    "-i src/SB/Core/x",
    "-i src/SB/Game",
    "-DGAMECUBE",
]

# Guards source that must NOT be in a matching build but must be in a runnable
# one. Retail contains reads of uninitialised stack that are harmless with its
# exact frame contents, are not harmless with ours, and will not be harmless in
# a PC port either -- see "Latent retail bugs" in docs/PCPORT.md. The fix has to be
# absent from the matching build, because adding it changes codegen, and
# present everywhere else.
if config.non_matching:
    cflags_bfbb.append("-DNON_MATCHING")

config.linker_version = "GC/2.0p1"

# The SB library is built with a patched CodeWarrior that narrows an
# over-aggressive may-alias inference in the instruction scheduler; see
# tools/patch_compiler.py. Derived from the stock compiler during the build.
PATCHED_COMPILER = "GC/2.0p1a"


# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "src_dir": "src/",
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_dolphin,
        "progress_category": "sdk",
        "host": True,
        "objects": objects,
    }

# Helper function for MSL libraries
def mslLib(lib_name: str, extra_cflags: List[str], objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "src_dir": "src/PowerPC_EABI_Support/src",
        "mw_version": "GC/2.6",
        "cflags": cflags_runtime + extra_cflags,
        "progress_category": "msl",
        "host": True,
        "objects": objects,
    }

def trkLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "src_dir": "src/runtime_libs",
        "mw_version": "GC/2.6",
        "cflags": cflags_runtime,
        "progress_category": "msl",
        "host": True,
        "objects": objects,
    }

# Helper function for RenderWare libraries
def RenderWareLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "src_dir": "src",
        # Not GC/1.3.2. Sweeping every available compiler over the seven rwsdk
        # units that have real source, 2.0p1 wins or ties every one of them and
        # beats 1.3.2 by ten functions overall - bacamera 9 -> 14, baworobj
        # 35 -> 37, baframe/baclump/bageomet +1 each.
        "mw_version": PATCHED_COMPILER,
        "cflags": cflags_renderware,
        "progress_category": "RW",
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "progress_category": "game",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "SB",
        "mw_version": PATCHED_COMPILER,
        "cflags": cflags_bfbb,
        "progress_category": "game",
        "objects": [
            Object(NonMatching, "SB/Core/x/xAnim.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xBase.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xbinio.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xBound.cpp"),
            Object(NonMatching, "SB/Core/x/xCamera.cpp"),
            Object(NonMatching, "SB/Core/x/xClimate.cpp"),
            Object(NonMatching, "SB/Core/x/xCollide.cpp",  extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xCollideFast.cpp"),
            Object(Matching, "SB/Core/x/xColor.cpp"),
            Object(Matching, "SB/Core/x/xCounter.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xCutscene.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xDebug.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xEnt.cpp", extra_cflags=["-sym on"]),
            Object(Equivalent, "SB/Core/x/xEntDrive.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xEntMotion.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xEnv.cpp"),
            Object(Matching, "SB/Core/x/xEvent.cpp"),
            Object(Matching, "SB/Core/x/xFFX.cpp"),
            Object(Matching, "SB/Core/x/xFog.cpp"),
            Object(NonMatching, "SB/Core/x/xFont.cpp"),
            Object(NonMatching, "SB/Core/x/xFX.cpp"),
            Object(Matching, "SB/Core/x/xGroup.cpp"),
            Object(Matching, "SB/Core/x/xhipio.cpp"),
            Object(NonMatching, "SB/Core/x/xHud.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xHudFontMeter.cpp"),
            Object(NonMatching, "SB/Core/x/xHudMeter.cpp"),
            Object(Matching, "SB/Core/x/xHudModel.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xHudUnitMeter.cpp", extra_cflags=["-sym on   "]),
            Object(Matching, "SB/Core/x/xIni.cpp"),
            Object(Matching, "SB/Core/x/xMath.cpp"),
            Object(Matching, "SB/Core/x/xMath2.cpp"),
            Object(NonMatching, "SB/Core/x/xMath3.cpp"),
            Object(NonMatching, "SB/Core/x/xMemMgr.cpp"),
            Object(NonMatching, "SB/Core/x/xModel.cpp"),
            Object(Matching, "SB/Core/x/xMorph.cpp"),
            Object(Equivalent, "SB/Core/x/xMovePoint.cpp"),
            Object(Matching, "SB/Core/x/xordarray.cpp"),
            Object(NonMatching, "SB/Core/x/xPad.cpp"),
            Object(Matching, "SB/Core/x/xPar.cpp"),
            Object(Matching, "SB/Core/x/xParCmd.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xParGroup.cpp"),
            Object(Matching, "SB/Core/x/xParMgr.cpp"),
            Object(Matching, "SB/Core/x/xPartition.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xpkrsvc.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xQuickCull.cpp"),
            Object(Matching, "SB/Core/x/xsavegame.cpp"),
            Object(NonMatching, "SB/Core/x/xScene.cpp",  extra_cflags=["-sym on"]),
            Object(Equivalent, "SB/Core/x/xScrFx.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xserializer.cpp"),
            Object(NonMatching, "SB/Core/x/xSFX.cpp"),
            Object(NonMatching, "SB/Core/x/xShadow.cpp"),
            Object(Matching, "SB/Core/x/xSnd.cpp"),
            Object(NonMatching, "SB/Core/x/xSpline.cpp"),
            Object(Equivalent, "SB/Core/x/xstransvc.cpp"),
            Object(NonMatching, "SB/Core/x/xString.cpp"),
            Object(Matching, "SB/Core/x/xSurface.cpp"),
            Object(Matching, "SB/Core/x/xTimer.cpp"),
            Object(Matching, "SB/Core/x/xTRC.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xutil.cpp"),
            Object(Matching, "SB/Core/x/xVec3.cpp"),
            Object(Matching, "SB/Game/zActionLine.cpp"),
            Object(Matching, "SB/Game/zAnimList.cpp"),
            Object(Equivalent, "SB/Game/zAssetTypes.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zCamera.cpp"),
            Object(Matching, "SB/Game/zConditional.cpp"),
            Object(NonMatching, "SB/Game/zCutsceneMgr.cpp"),
            Object(Matching, "SB/Game/zDispatcher.cpp"),
            Object(Matching, "SB/Game/zEGenerator.cpp"),
            Object(Matching, "SB/Game/zEnt.cpp"),
            Object(Matching, "SB/Game/zEntButton.cpp"),
            Object(NonMatching, "SB/Game/zEntCruiseBubble.cpp"),
            Object(Matching, "SB/Game/zEntDestructObj.cpp"),
            Object(NonMatching, "SB/Game/zEntHangable.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zEntPickup.cpp"),
            Object(NonMatching, "SB/Game/zEntPlayer.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zEntSimpleObj.cpp"),
            Object(Matching, "SB/Game/zEntTrigger.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zEnv.cpp"),
            Object(Matching, "SB/Game/zEvent.cpp"),
            Object(Matching, "SB/Game/zFeet.cpp"),
            Object(Matching, "SB/Game/zFMV.cpp"),
            Object(NonMatching, "SB/Game/zFX.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zGame.cpp"),
            Object(Matching, "SB/Game/zGameExtras.cpp", extra_cflags=["-sym on"]),
            Object(Equivalent, "SB/Game/zGameState.cpp"),
            Object(NonMatching, "SB/Game/zGust.cpp"),
            Object(NonMatching, "SB/Game/zHud.cpp"),
            Object(NonMatching, "SB/Game/zLasso.cpp"),
            Object(Matching, "SB/Game/zLight.cpp"),
            Object(Matching, "SB/Game/zLightEffect.cpp"),
            Object(NonMatching, "SB/Game/zLightning.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zLOD.cpp"),
            Object(NonMatching, "SB/Game/zMain.cpp"),
            Object(Matching, "SB/Game/zMenu.cpp"),
            Object(Matching, "SB/Game/zMovePoint.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zMusic.cpp"),
            Object(Equivalent, "SB/Game/zParCmd.cpp"),
            Object(Matching, "SB/Game/zParEmitter.cpp"),
            Object(Matching, "SB/Game/zPendulum.cpp"),
            Object(Matching, "SB/Game/zPickupTable.cpp"),
            Object(Matching, "SB/Game/zPlatform.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zPortal.cpp"),
            Object(Matching, "SB/Game/zRenderState.cpp"),
            Object(Matching, "SB/Game/zRumble.cpp"),
            Object(Equivalent, "SB/Game/zSaveLoad.cpp"),
            Object(NonMatching, "SB/Game/zScene.cpp"),
            Object(Matching, "SB/Game/zScript.cpp"),
            Object(NonMatching, "SB/Game/zSurface.cpp"),
            Object(NonMatching, "SB/Game/zThrown.cpp"),
            Object(NonMatching, "SB/Game/zUI.cpp"),
            Object(NonMatching, "SB/Game/zUIFont.cpp"),
            Object(Matching, "SB/Game/zVar.cpp"),
            Object(Matching, "SB/Game/zVolume.cpp"),
            Object(Equivalent, "SB/Core/gc/iAnim.cpp"),
            Object(NonMatching, "SB/Core/gc/iAnimSKB.cpp"),
            Object(Matching, "SB/Core/x/iCamera.cpp"),
            Object(NonMatching, "SB/Core/gc/iCollide.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/gc/iCollideFast.cpp"),
            Object(Matching, "SB/Core/gc/iDraw.cpp"),
            Object(Matching, "SB/Core/gc/iEnv.cpp"),
            Object(NonMatching, "SB/Core/gc/iFile.cpp"),
            Object(Equivalent, "SB/Core/gc/iFMV.cpp", extra_cflags=["-DGEKKO"]),
            Object(NonMatching, "SB/Core/gc/iFX.cpp"),
            Object(Matching, "SB/Core/gc/iLight.cpp"),
            Object(Matching, "SB/Core/gc/iMath.cpp"),
            Object(NonMatching, "SB/Core/gc/iMath3.cpp"),
            Object(Matching, "SB/Core/gc/iMemMgr.cpp"),
            Object(Matching, "SB/Core/gc/iMix.c"),
            Object(NonMatching, "SB/Core/gc/iModel.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/gc/iMorph.cpp"),
            Object(Equivalent, "SB/Core/gc/iPad.cpp"),
            Object(NonMatching, "SB/Core/gc/iParMgr.cpp"),
            Object(NonMatching, "SB/Core/gc/isavegame.cpp"),
            Object(NonMatching, "SB/Core/gc/iScrFX.cpp"),
            Object(NonMatching, "SB/Core/gc/iSnd.cpp"),
            Object(NonMatching, "SB/Core/gc/iSystem.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/gc/iTime.cpp"),
            Object(NonMatching, "SB/Core/gc/ngcrad3d.c", extra_cflags=["-DGEKKO"]),
            Object(Matching, "SB/Game/zNPCGoals.cpp"),
            Object(Matching, "SB/Game/zNPCGoalCommon.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCGoalStd.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCGoalRobo.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCGoalTiki.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCMessenger.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCMgr.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCTypes.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeCommon.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCTypeRobot.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeVillager.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCTypeAmbient.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCTypeTiki.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xBehaveMgr.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xBehaviour.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xBehaveGoalSimple.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xSkyDome.cpp"),
            Object(Matching, "SB/Core/x/xRMemData.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xFactory.cpp"),
            Object(Matching, "SB/Core/x/xNPCBasic.cpp"),
            Object(NonMatching, "SB/Game/zEntPlayerBungeeState.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zCollGeom.cpp"),
            Object(Matching, "SB/Core/x/xParSys.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xParEmitter.cpp"),
            Object(Matching, "SB/Core/x/xVolume.cpp"),
            Object(Matching, "SB/Core/x/xParEmitterType.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xRenderState.cpp"),
            Object(NonMatching, "SB/Game/zEntPlayerOOBState.cpp", extra_cflags=["-sym on"]),
            Object(Equivalent, "SB/Core/x/xClumpColl.cpp"),
            Object(Matching, "SB/Core/x/xEntBoulder.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xGrid.cpp"),
            Object(Matching, "SB/Core/x/xJSP.cpp"),
            Object(Matching, "SB/Core/x/xLightKit.cpp"),
            Object(Matching, "SB/Game/zCamMarker.cpp"),
            Object(Matching, "SB/Game/zGoo.cpp"),
            Object(Matching, "SB/Game/zGrid.cpp"),
            Object(Matching, "SB/Game/zNPCGoalScript.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCSndTable.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCSndLists.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeDuplotron.cpp"),
            Object(Equivalent, "SB/Core/x/xModelBucket.cpp"),
            Object(NonMatching, "SB/Game/zShrapnel.cpp"),
            Object(Matching, "SB/Game/zNPCGoalDuplotron.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCSpawner.cpp"),
            Object(Matching, "SB/Game/zEntTeleportBox.cpp"),
            Object(Matching, "SB/Game/zBusStop.cpp"),
            Object(NonMatching, "SB/Game/zNPCSupport.cpp"),
            Object(NonMatching, "SB/Game/zTalkBox.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zTextBox.cpp"),
            Object(Matching, "SB/Game/zTaskBox.cpp"),
            Object(Matching, "SB/Core/gc/iCutscene.cpp"),
            Object(Matching, "SB/Game/zNPCTypeTest.cpp"),
            Object(Matching, "SB/Game/zNPCTypeSubBoss.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeBoss.cpp"),
            Object(Matching, "SB/Game/zNPCGoalVillager.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCGoalSubBoss.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xShadowSimple.cpp"),
            Object(Matching, "SB/Core/x/xUpdateCull.cpp"),
            Object(NonMatching, "SB/Game/zDiscoFloor.cpp"),
            Object(Matching, "SB/Game/zNPCTypeBossSandy.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCTypeKingJelly.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCGoalBoss.cpp"),
            Object(Matching, "SB/Game/zNPCTypePrawn.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Game/zNPCTypeBossSB1.cpp"),
            Object(Matching, "SB/Game/zNPCTypeBossSB2.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xJaw.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeBossPatrick.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCTypeBossPlankton.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zParPTank.cpp"),
            Object(Matching, "SB/Game/zTaxi.cpp"),
            Object(NonMatching, "SB/Game/zNPCTypeDutchman.cpp"),
            Object(Matching, "SB/Game/zCameraFly.cpp"),
            Object(Matching, "SB/Core/x/xCurveAsset.cpp"),
            Object(Matching, "SB/Core/x/xDecal.cpp", extra_cflags=["-sym on"]),
            Object(Matching, "SB/Core/x/xLaserBolt.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zCameraTweak.cpp"),
            Object(Matching, "SB/Core/x/xPtankPool.cpp"),
            Object(Equivalent, "SB/Core/gc/iTRC.cpp"),
            Object(NonMatching, "SB/Game/zNPCSupplement.cpp"),
            Object(NonMatching, "SB/Game/zNPCGlyph.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCHazard.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zNPCGoalAmbient.cpp"),
            Object(NonMatching, "SB/Game/zNPCFXCinematic.cpp"),
            Object(Matching, "SB/Core/x/xHudText.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Game/zCombo.cpp", extra_cflags=["-sym on"]),
            Object(NonMatching, "SB/Core/x/xCM.cpp"),
        ],
    },
    {
        "lib": "binkngc",
        "mw_version": "ProDG/3.5",
        "cflags": cflags_bink,
        "progress_category": "bink",
        "objects": [
            Object(NonMatching, "bink/src/sdk/decode/ngc/binkngc.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcsnd.c", extra_cflags=["-G0"]),
            Object(NonMatching, "bink/src/sdk/decode/binkread.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcfile.c"),
            Object(NonMatching, "bink/src/sdk/decode/yuv.cpp"),
            Object(NonMatching, "bink/src/sdk/decode/binkacd.c"),
            Object(Matching, "bink/shared/time/radcb.c"),
            Object(NonMatching, "bink/src/sdk/decode/expand.c"),
            Object(Matching, "bink/src/sdk/popmal.c"),
            Object(NonMatching, "bink/src/sdk/decode/ngc/ngcrgb.c"),
            Object(Matching, "bink/src/sdk/decode/ngc/ngcyuy2.c"),
            Object(Matching, "bink/src/sdk/varbits.c"),
            Object(Matching, "bink/src/sdk/fft.c"),
            Object(NonMatching, "bink/src/sdk/dct.c"),
            Object(NonMatching, "bink/src/sdk/bitplane.c"),
        ],
    },
    DolphinLib(
        "ai",
        [
            Object(Matching, "dolphin/src/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "amcstubs",
        [
            Object(Matching, "dolphin/src/amcstubs/AmcExi2Stubs.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(Matching, "dolphin/src/ar/ar.c"),
            Object(Matching, "dolphin/src/ar/arq.c")
        ]
    ),
    DolphinLib(
        "ax",
        [
            Object(Matching, "dolphin/src/ax/AX.c"),
            Object(Matching, "dolphin/src/ax/AXAlloc.c"),
            Object(Matching, "dolphin/src/ax/AXAux.c"),
            Object(Matching, "dolphin/src/ax/AXCL.c"),
            Object(Matching, "dolphin/src/ax/AXOut.c"),
            Object(Matching, "dolphin/src/ax/AXSPB.c"),
            Object(Matching, "dolphin/src/ax/AXVPB.c"),
            Object(Matching, "dolphin/src/ax/AXComp.c"),
            Object(Matching, "dolphin/src/ax/DSPCode.c"),
            Object(Matching, "dolphin/src/ax/AXProf.c"),
        ],
    ),
    DolphinLib(
        "base",
        [
            Object(Matching, "dolphin/src/base/PPCArch.c")
        ]
    ),
    DolphinLib(
        "card",
        [
            Object(Matching, "dolphin/src/card/CARDBios.c"),
            Object(Matching, "dolphin/src/card/CARDUnlock.c"),
            Object(Matching, "dolphin/src/card/CARDRdwr.c"),
            Object(Matching, "dolphin/src/card/CARDBlock.c"),
            Object(Matching, "dolphin/src/card/CARDDir.c"),
            Object(Matching, "dolphin/src/card/CARDCheck.c"),
            Object(Matching, "dolphin/src/card/CARDMount.c"),
            Object(Matching, "dolphin/src/card/CARDFormat.c"),
            Object(Matching, "dolphin/src/card/CARDOpen.c"),
            Object(Matching, "dolphin/src/card/CARDCreate.c"),
            Object(Matching, "dolphin/src/card/CARDRead.c"),
            Object(Matching, "dolphin/src/card/CARDWrite.c"),
            Object(Matching, "dolphin/src/card/CARDDelete.c"),
            Object(Matching, "dolphin/src/card/CARDStat.c"),
            Object(Matching,"dolphin/src/card/CARDStatEx.c"),
            Object(Matching, "dolphin/src/card/CARDNet.c"),
        ]
    ),
    DolphinLib(
        "db",
        [
            Object(Matching, "dolphin/src/db/db.c"),
        ]
    ),
    DolphinLib(
        "dsp",
        [
            Object(Matching, "dolphin/src/dsp/dsp.c"),
            Object(Matching, "dolphin/src/dsp/dsp_debug.c"),
            Object(Matching, "dolphin/src/dsp/dsp_task.c")
        ]
    ),
    DolphinLib(
        "dvd",
        [
            Object(Matching, "dolphin/src/dvd/dvdlow.c"),
            Object(Matching, "dolphin/src/dvd/dvdfs.c"),
            Object(Matching, "dolphin/src/dvd/dvd.c"),
            Object(Matching, "dolphin/src/dvd/dvdqueue.c"),
            Object(Matching, "dolphin/src/dvd/dvderror.c"),
            Object(Matching, "dolphin/src/dvd/dvdidutils.c"),
            Object(Matching, "dolphin/src/dvd/dvdFatal.c"),
            Object(Matching, "dolphin/src/dvd/emu_level2/fstload.c"),
        ],
    ),
    DolphinLib(
        "exi",
        [
            Object(Matching, "dolphin/src/exi/EXIBios.c"),
            Object(Matching, "dolphin/src/exi/EXIUart.c")
        ]
    ),
    DolphinLib(
        "gx",
        [
            Object(Matching, "dolphin/src/gx/GXInit.c"),
            Object(Matching, "dolphin/src/gx/GXFifo.c"),
            Object(Matching, "dolphin/src/gx/GXAttr.c"),
            Object(Matching, "dolphin/src/gx/GXMisc.c"),
            Object(Matching, "dolphin/src/gx/GXGeometry.c"),
            Object(Matching, "dolphin/src/gx/GXFrameBuf.c"),
            Object(Matching, "dolphin/src/gx/GXLight.c"),
            Object(Matching, "dolphin/src/gx/GXTexture.c"),
            Object(Matching, "dolphin/src/gx/GXBump.c"),
            Object(Matching, "dolphin/src/gx/GXTev.c"),
            Object(Matching, "dolphin/src/gx/GXPixel.c"),
            Object(Matching, "dolphin/src/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/src/gx/GXTransform.c"),
            Object(Matching, "dolphin/src/gx/GXPerf.c")
        ]
    ),
    DolphinLib(
        "mtx",
        [
            Object(Matching, "dolphin/src/mtx/mtx.c"),
            Object(Matching, "dolphin/src/mtx/mtx44.c"),
        ]
    ),
    DolphinLib(
        "OdemuExi2",
        [
            Object(Matching, "dolphin/src/OdemuExi2/DebuggerDriver.c", extra_cflags=["-inline on, deferred"])
        ]
    ),
    DolphinLib(
        "odenotstub",
        [
            Object(Matching, "dolphin/src/odenotstub/odenotstub.c")
        ]
    ),
    DolphinLib(
        "os",
        [
            Object(Matching, "dolphin/src/os/OS.c"),
            Object(Matching, "dolphin/src/os/OSAlarm.c"),
            Object(Matching, "dolphin/src/os/OSAlloc.c"),
            Object(Matching, "dolphin/src/os/OSArena.c"),
            Object(Matching, "dolphin/src/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/src/os/OSCache.c"),
            Object(Matching, "dolphin/src/os/OSContext.c"),
            Object(Matching, "dolphin/src/os/OSError.c"),
            Object(Matching, "dolphin/src/os/OSFont.c"),
            Object(Matching, "dolphin/src/os/OSInterrupt.c"),
            Object(Matching, "dolphin/src/os/OSLink.c"),
            Object(Matching, "dolphin/src/os/OSMemory.c"),
            Object(Matching, "dolphin/src/os/OSMutex.c"),
            Object(Matching, "dolphin/src/os/OSReboot.c"),
            Object(Matching, "dolphin/src/os/OSReset.c"),
            Object(Matching, "dolphin/src/os/OSResetSW.c"),
            Object(Matching, "dolphin/src/os/OSRtc.c"),
            Object(Matching, "dolphin/src/os/OSThread.c"),
            Object(Matching, "dolphin/src/os/OSTime.c"),
            Object(Matching, "dolphin/src/os/OSSync.c"),
            Object(Matching, "dolphin/src/os/init/__start.c"),
            Object(Matching, "dolphin/src/os/init/__ppc_eabi_init.cpp")
        ]
    ),
    DolphinLib(
        "pad",
        [
            Object(Matching, "dolphin/src/pad/Padclamp.c"),
            Object(Matching, "dolphin/src/pad/Pad.c")
        ]
    ),
    DolphinLib(
        "si",
        [
            Object(Matching, "dolphin/src/si/SIBios.c"),
            Object(Matching, "dolphin/src/si/SISamplingRate.c"),
        ]
    ),
    DolphinLib(
        "vi",
        [
            Object(Matching, "dolphin/src/vi/vi.c", extra_cflags=["-DMATCHING"]),
        ],
    ),
    mslLib(
        "Runtime.PPCEABI.H",
        [],
        [
            Object(Matching, "Runtime/__mem.c"),
            Object(Matching, "Runtime/__va_arg.c"),
            Object(Matching, "Runtime/global_destructor_chain.c"),
            Object(
                Matching,
                "Runtime/New.cp",
                mw_version="GC/2.6",
                cflags=[
                    flag
                    for flag in cflags_runtime
                    if flag not in ("-RTTI off", "-Cpp_exceptions off", "-inline auto", "-str reuse,pool,readonly")
                ]
                + ["-Cpp_exceptions on", "-RTTI on", "-inline auto,deferred", "-str reuse,nopool,readonly"],
            ),
            Object(
                Matching,
                "Runtime/NMWException.cp",
                mw_version="GC/2.0p1",
                cflags=[
                    flag
                    for flag in cflags_runtime
                    if flag not in ("-RTTI off", "-Cpp_exceptions off")
                ]
                + ["-Cpp_exceptions on", "-RTTI on", "-inline auto,deferred"],
            ),
            Object(Matching, "Runtime/CPlusLibPPC.cp"),
            Object(Matching, "Runtime/ptmf.c"),
            Object(Matching, "Runtime/runtime.c"),
            Object(Matching, "Runtime/__init_cpp_exceptions.cpp"),
            Object(
                Matching,
                "Runtime/Gecko_ExceptionPPC.cp",
                cflags=[
                    flag
                    for flag in cflags_runtime
                    if flag not in ("-RTTI off", "-Cpp_exceptions off", "-str reuse,pool,readonly")
                ]
                + ["-Cpp_exceptions on", "-RTTI on", "-str reuse,nopool,readonly"],
                extab_padding=[0x02, 0x55],
            ),
            Object(Matching, "Runtime/GCN_mem_alloc.c", extra_cflags=["-str reuse,nopool,readonly"]),
        ]
    ),
    mslLib(
        "MSL_C.PPCEABI.H",
        ["-str pool", "-opt level=0, peephole, schedule, nospace", "-inline off", "-sym on"],
        [
            Object(Matching, "MSL_C/PPC_EABI/abort_exit.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/alloc.c", cflags=cflags_msl_runtime_c),
            Object(Matching, "MSL_C/MSL_Common/ansi_files.c", mw_version="GC/1.3", cflags=cflags_msl_gc13_runtime),
            Object(Matching, "MSL_C/MSL_Common_Embedded/ansi_fp.c", cflags=cflags_msl_runtime_c),
            Object(Matching, "MSL_C/MSL_Common/arith.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/bsearch.c"),
            Object(Matching, "MSL_C/MSL_Common/buffer_io.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/PPC_EABI/critical_regions.gamecube.c"),
            Object(Matching, "MSL_C/MSL_Common/ctype.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/direct_io.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/errno.c"),
            Object(Matching, "MSL_C/MSL_Common/file_io.c", cflags=cflags_msl_runtime_c + ["-D_MSL_WIDE_CHAR"]),
            Object(Matching, "MSL_C/MSL_Common/FILE_POS.C", cflags=cflags_msl_runtime_cpp),
            Object(Matching, "MSL_C/MSL_Common/locale.c"),
            Object(Matching, "MSL_C/MSL_Common/mbstring.c", mw_version="GC/1.3", cflags=cflags_msl_gc13_runtime),
            Object(Matching, "MSL_C/MSL_Common/mem.c", mw_version="GC/1.3", cflags=cflags_msl_gc13_runtime),
            Object(Matching, "MSL_C/MSL_Common/mem_funcs.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/misc_io.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/printf.c", mw_version="GC/2.0p1", cflags=cflags_msl_runtime_c),
            Object(Matching, "MSL_C/MSL_Common/qsort.c", cflags=cflags_msl_runtime_c),
            Object(Matching, "MSL_C/MSL_Common/rand.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/scanf.c", cflags=cflags_runtime + ["-inline deferred"]),
            Object(Matching, "MSL_C/MSL_Common/signal.c", cflags=cflags_msl_gc13_runtime),
            Object(Matching, "MSL_C/MSL_Common/string.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/strtold.c", cflags=cflags_msl_runtime_c),
            Object(Matching, "MSL_C/MSL_Common/strtoul.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/float.c"),
            Object(Matching, "MSL_C/MSL_Common/char_io.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common/wchar_io.c", cflags=cflags_runtime),
            Object(Matching, "MSL_C/MSL_Common_Embedded/uart_console_io_gcn.c", cflags=cflags_runtime)
        ]
    ),
    mslLib(
        "fdlibm.PPCEABI.H",
        [],
        [
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_acos.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_exp.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_fmod.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_log.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_pow.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_cos.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_sin.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_tan.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ceil.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_copysign.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_cos.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_floor.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ldexp.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_modf.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_sin.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_tan.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_acos.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_asin.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_atan2.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_exp.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_fmod.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_log.c"),
            Object(Matching, "MSL_C/MSL_Common_Embedded/Math/Double_precision/w_pow.c"),
            Object(Matching, "MSL_C/PPC_EABI/math_ppc.c"),
        ]
    ),
    trkLib(
        "TRK_MINNOW_DOLPHIN",
        [
            Object(Matching, "debugger/embedded/MetroTRK/Portable/mainloop.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/nubevent.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/nubassrt.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/nubinit.c", extra_cflags=["-sdata 0", "-sdata2 0"]),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/msg.c", mw_version="GC/1.3", cflags=cflags_trk),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/msgbuf.c", mw_version="GC/1.3", cflags=cflags_trk),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/serpoll.c", extra_cflags=["-sdata 0", "-sdata2 0"]),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/dispatch.c", extra_cflags=["-sdata 0", "-sdata2 0"]),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/msghndlr.c", mw_version="GC/2.6", cflags=cflags_trk),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/support.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/mutex_TRK.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/notify.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/main_TRK.c", extra_cflags=["-sdata 0", "-sdata2 0"]),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/mem_TRK.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Portable/string_TRK.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Processor/ppc/Generic/flush_cache.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Processor/ppc/Generic/__exception.s"),
            Object(Matching, "debugger/embedded/MetroTRK/Processor/ppc/Generic/targimpl.c", mw_version="GC/2.6", cflags=[*cflags_trk, "-gccinc", "-common off"]),
            Object(Matching, "debugger/embedded/MetroTRK/Processor/ppc/Export/targsupp.s"),
            Object(Matching, "debugger/embedded/MetroTRK/Processor/ppc/Generic/mpc_7xx_603e.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk.c", mw_version="GC/2.6", cflags=cflags_trk),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/usr_put.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk_glue.c", mw_version="GC/2.6", cflags=cflags_trk),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/targcont.c"),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/target_options.c", extra_cflags=["-sdata 0", "-sdata2 0"]),
            Object(Matching, "debugger/embedded/MetroTRK/Os/dolphin/UDP_Stubs.c"),
             Object(
                Matching,
                "debugger/embedded/MetroTRK/Export/mslsupp.c",
                mw_version="GC/2.6",
                cflags=cflags_trk,
            ),

            Object(Matching, "gamedev/cust_connection/cc/exi2/GCN/EXI2_DDH_GCN/main.c"),
            Object(Matching, "gamedev/cust_connection/utils/common/CircleBuffer.c"),
            Object(Matching, "gamedev/cust_connection/cc/exi2/GCN/EXI2_GDEV_GCN/main.c"),
            Object(Matching, "gamedev/cust_connection/utils/common/MWTrace.c"),
            Object(Matching, "gamedev/cust_connection/utils/gc/MWCriticalSection_gc.cpp"),
        ]
    ),
    mslLib(
        "MSL_C.PPCEABI.bare.H",
        [],
        [
            Object(Matching, "MSL_C/MSL_Common/extras.c")
        ]
    ),
    RenderWareLib(
        "rpcollis",
        [
            Object(NonMatching, "rwsdk/plugin/collis/ctgeom.c"),
            Object(Matching, "rwsdk/plugin/collis/ctworld.c"),
            Object(Matching, "rwsdk/plugin/collis/ctbsp.c"),
            Object(NonMatching, "rwsdk/plugin/collis/rpcollis.c"),
        ],
    ),
    RenderWareLib(
        "rphanim",
        [
            Object(NonMatching, "rwsdk/plugin/hanim/stdkey.c"),
            Object(Matching, "rwsdk/plugin/hanim/rphanim.c"),
        ],
    ),
    RenderWareLib(
        "rpmatfx",
        [
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/effectPipesGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnData.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/gcn/multiTexGcnPipe.c"),
            Object(Matching, "rwsdk/plugin/matfx/gcn/multiTexGcn.c"),
            Object(NonMatching, "rwsdk/plugin/matfx/multiTex.c"),
            Object(Matching, "rwsdk/plugin/matfx/multiTexEffect.c"),
            Object(Matching, "rwsdk/plugin/matfx/rpmatfx.c"),
        ],
    ),
    RenderWareLib(
        "rpptank",
        [
            Object(NonMatching, "rwsdk/plugin/ptank/rpptank.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcncallbacks.c"),
            Object(NonMatching, "rwsdk/plugin/ptank/gcn/ptankgcnrender.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcntransforms.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_ppm.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_ppm.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_nr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_nr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_nr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_nr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_cs_ppr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_cs_ppr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_nc_pps_ppr.c"),
            Object(Matching, "rwsdk/plugin/ptank/gcn/ptankgcn_cc_pps_ppr.c"),
        ],
    ),
    RenderWareLib(
        "rpskinmatfx",
        [
            Object(Matching, "rwsdk/plugin/skin2/bsplit.c"),
            Object(Matching, "rwsdk/plugin/skin2/rpskin.c"),
            Object(NonMatching, "rwsdk/plugin/skin2/gcn/skingcn.c"),
            Object(Matching, "rwsdk/plugin/skin2/gcn/skinstream.c"),
            Object(Matching, "rwsdk/plugin/skin2/gcn/instance/instanceskin.c"),
            Object(Matching, "rwsdk/plugin/skin2/gcn/skinmatrixblend.c"),
            Object(Matching, "rwsdk/plugin/skin2/gcn/skingcnasm.c"),
            Object(Matching, "rwsdk/plugin/skin2/gcn/skingcng.c"),
        ],
    ),
    RenderWareLib(
        "rpusrdat",
        [
            Object(NonMatching, "rwsdk/plugin/userdata/rpusrdat.c"),
        ],
    ),
    RenderWareLib(
        "rpworld",
        [
            Object(NonMatching, "rwsdk/world/babinwor.c"),
            Object(Matching, "rwsdk/world/baclump.c"),
            Object(NonMatching, "rwsdk/world/bageomet.c"),
            Object(NonMatching, "rwsdk/world/balight.c"),
            Object(NonMatching, "rwsdk/world/bamateri.c"),
            Object(NonMatching, "rwsdk/world/bamatlst.c"),
            Object(Matching, "rwsdk/world/bamesh.c"),
            Object(NonMatching, "rwsdk/world/bameshop.c"),
            Object(Matching, "rwsdk/world/basector.c"),
            Object(Matching, "rwsdk/world/baworld.c"),
            Object(Matching, "rwsdk/world/baworobj.c"),
            Object(Matching, "rwsdk/world/pipe/p2/bapipew.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/gcpipe.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/vtxfmt.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/wrldpipe.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeAtomicAllInOne.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/nodeGameCubeWorldSectorAllInOne.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/gclights.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/gcmorph.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/native.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/setup.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geomcond.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/geominst.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/ibuffer.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/instance/instancegeom.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/instance/instanceworld.c"),
            Object(NonMatching, "rwsdk/world/pipe/p2/gcn/instance/itools.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/instance/vbuffer.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/instance/vtools.c"),
            Object(Matching, "rwsdk/world/pipe/p2/gcn/instance/vtxdesc.c"),
        ],
    ),
    RenderWareLib(
        "rtanim",
        [
            Object(Matching, "rwsdk/tool/anim/rtanim.c"),
        ],
    ),
    RenderWareLib(
        "rtintsec",
        [
            Object(Matching, "rwsdk/tool/intsec/rtintsec.c"),
        ],
    ),
    RenderWareLib(
        "rtslerp",
        [
            Object(NonMatching, "rwsdk/tool/slerp/rtslerp.c"),
        ],
    ),
    RenderWareLib(
        "rwcore",
        [
            Object(Matching, "rwsdk/src/plcore/babinary.c"),
            Object(Matching, "rwsdk/src/plcore/bacolor.c"),
            Object(Matching, "rwsdk/src/plcore/baerr.c"),
            Object(Matching, "rwsdk/src/plcore/bafsys.c"),
            Object(Matching, "rwsdk/src/plcore/baimmedi.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamatrix.c"),
            Object(NonMatching, "rwsdk/src/plcore/bamemory.c"),
            Object(Matching, "rwsdk/src/plcore/baresour.c"),
            Object(Matching, "rwsdk/src/plcore/bastream.c"),
            Object(Matching, "rwsdk/src/plcore/batkbin.c"),
            Object(Matching, "rwsdk/src/plcore/batkreg.c"),
            Object(NonMatching, "rwsdk/src/plcore/bavector.c"),
            Object(Matching, "rwsdk/src/plcore/resmem.c"),
            Object(Matching, "rwsdk/src/plcore/rwstring.c"),
            Object(Matching, "rwsdk/os/gcn/osintf.c"),
            Object(Matching, "rwsdk/src/babbox.c"),
            Object(Matching, "rwsdk/src/babincam.c"),
            Object(NonMatching, "rwsdk/src/babinfrm.c"),
            Object(NonMatching, "rwsdk/src/babintex.c"),
            Object(NonMatching, "rwsdk/src/bacamera.c"),
            Object(Matching, "rwsdk/src/badevice.c"),
            Object(Matching, "rwsdk/src/baframe.c"),
            Object(NonMatching, "rwsdk/src/baimage.c"),
            Object(Matching, "rwsdk/src/baimras.c"),
            Object(Matching, "rwsdk/src/baraster.c"),
            Object(NonMatching, "rwsdk/src/baresamp.c"),
            Object(Matching, "rwsdk/src/basync.c"),
            Object(Matching, "rwsdk/src/batextur.c"),
            Object(Matching, "rwsdk/src/batypehf.c"),
            Object(NonMatching, "rwsdk/driver/common/palquant.c"),
            Object(Matching, "rwsdk/driver/gcn/dl2drend.c"),
            Object(Matching, "rwsdk/driver/gcn/dlconvrt.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dldevice.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dlraster.c"),
            Object(Matching, "rwsdk/driver/gcn/dlrendst.c"),
            Object(Matching, "rwsdk/driver/gcn/dlsprite.c"),
            Object(NonMatching, "rwsdk/driver/gcn/dltexdic.c"),
            Object(Matching, "rwsdk/driver/gcn/dltextur.c"),
            Object(Matching, "rwsdk/driver/gcn/dltoken.c"),
            Object(Matching, "rwsdk/src/pipe/p2/baim3d.c"),
            Object(Matching, "rwsdk/src/pipe/p2/bapipe.c"),
            Object(Matching, "rwsdk/src/pipe/p2/p2altmdl.c"),
            Object(Matching, "rwsdk/src/pipe/p2/p2core.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2define.c"),
            Object(NonMatching, "rwsdk/src/pipe/p2/p2dep.c"),
            Object(Matching, "rwsdk/src/pipe/p2/p2heap.c"),
            Object(Matching, "rwsdk/src/pipe/p2/p2renderstate.c"),
            Object(Matching, "rwsdk/src/pipe/p2/p2resort.c"),
            Object(Matching, "rwsdk/src/pipe/p2/gcn/im3dpipe.c"),
            Object(Matching, "rwsdk/src/pipe/p2/gcn/nodeDolphinSubmitNoLight.c"),
        ],
    ),
]


# Optional callback to adjust link order. This can be used to add, remove, or reorder objects.
# This is called once per module, with the module ID and the current link order.
#
# For example, this adds "dummy.c" to the end of the DOL link order if configured with --non-matching.
# "dummy.c" *must* be configured as a Matching (or Equivalent) object in order to be linked.
def link_order_callback(module_id: int, objects: List[str]) -> List[str]:
    # Don't modify the link order for matching builds
    if not config.non_matching:
        return objects
    if module_id == 0:  # DOL
        return objects + ["dummy.c"]
    return objects

# Uncomment to enable the link order callback.
# config.link_order_callback = link_order_callback


# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK Code"),
    ProgressCategory("msl", "MSL"),
    ProgressCategory("RW", "Renderware SDK"),
    ProgressCategory("bink", "Bink SDK"),
]
config.progress_each_module = args.verbose
config.progress_report_args = [
    "--deduplicate",
]

# Compilers are fetched by a ninja rule, so the patched variant has to be
# derived as a build step rather than at configure time. Objects already carry
# order_only="pre-compile".
compilers_dir = (
    Path(config.compilers_path)
    if config.compilers_path
    else config.build_dir / "compilers"
)
# AliasPatch.c lives in the (local, unpublished) compiler decomp, so it is an
# absolute path outside this tree and is absent on any machine that has only
# the bfbb repo -- CI included. Treat it as a dependency when it is there; when
# it is not, patch_compiler.py must derive from the checked-in blob instead.
ALIASPATCH_SRC = aliaspatch_link.SRC

config.custom_build_rules = [
    {
        "name": "patch_compiler",
        "command": "$python tools/patch_compiler.py $out",
        "description": "PATCH $out",
    }
]
config.custom_build_steps = {
    "pre-compile": [
        {
            "outputs": [compilers_dir / PATCHED_COMPILER / "mwcceppc.exe"],
            "rule": "patch_compiler",
            # The script is a real input: editing the payload must re-derive the
            # compiler, which in turn rebuilds every object (see project.py).
            # When the compilers are downloaded that directory is a ninja
            # target; wait for it. With --compilers it already exists on disk.
            # The compiler is now derived from C (AliasPatch.c in the
            # mwcc-gc repo) rather than from hand-assembled cave bytes, so all
            # four of these are real inputs. Listing only patch_compiler.py
            # meant editing the payload left a stale compiler behind and every
            # object silently kept its old bytes.
            "implicit": [
                Path("tools") / "patch_compiler.py",
                Path("tools") / "aliaspatch_link.py",
                Path("tools") / "aliaspatch_asm.py",
            ]
            + ([Path(ALIASPATCH_SRC)] if Path(ALIASPATCH_SRC).exists() else [])
            + ([compilers_dir] if config.compilers_path is None else []),
        }
    ]
}

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress and write progress.json
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
