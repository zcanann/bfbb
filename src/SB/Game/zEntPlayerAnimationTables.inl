// Animation-table definitions belong to this header in the original debug data.
// TU-private definitions; public declarations remain in the shared header.

xAnimTable* zSandy_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("Sandy", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle01b", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle01c", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle05", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive_sleep", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat03", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeat04", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatGoo", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpLift01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandRun01", 0x20, 0x0006, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LCopterHeadUp01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LCopter01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideJumpStart01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideJumpApex01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideFall01", 0x10, 0x100a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideLand01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TailSlideDJumpApex01", 0x20, 0x100a, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingCatch01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingCatch02", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwing", 0x10, 0x0040, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoSwingRelease", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpMelee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Melee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoWindup", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoThrow", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoFly", 0x10, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoDestroy", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoAboutToDestroy", 0x10, 0x0080, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyRope", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyFight", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyWin", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LassoEnemyLose", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(animTable, "LandRun01", "Run01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01b Idle01c Idle02 Idle04 Idle05 Inactive_sleep Inactive01 Inactive02 Land01 LandHigh01",
        "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.1, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 SlipRun01",
        "LassoSwingCatch01", LassoSwingGroundedBeginCheck, LassoSwingGroundedBeginCB, 0, 0, 0.0,
        0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpApex01 Fall01 DJumpStart01 DJumpApex01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01 TailSlideDJumpApex01 LCopter01 LCopterHeadUp01 BounceStart01 BounceLift01 BounceApex01",
        "LassoSwingCatch01", LassoSwingBeginCheck, LassoSwingBeginCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
        NULL);

    xAnimTableNewTransition(animTable, "LassoSwingCatch01", "LassoSwingCatch02", NULL,
                            LassoSwingTossCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch02", "LassoSwing", NULL, LassoSwingCB, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "SlipIdle01", IdleSlipCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Idle01",
                            IdleCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Walk01",
                            WalkCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Run01",
                            RunAnyCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "RunOutOfWorld01", RunOutOfWorldCheck, LassoSwingGroundedCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing",
                            "SlipRun01", RunSlipCheck, LassoSwingGroundedCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoSwingCatch01 LassoSwingCatch02 LassoSwing", "Fall01",
                            LassoSwingReleaseCheck, LassoSwingReleaseCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);

    xAnimTableNewTransition(animTable, "LassoWindup", "LassoThrow", NULL, LassoThrowCB, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "LassoThrow", "LassoFly", NULL, LassoFlyCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "LassoEnemyRope", "LassoEnemyFight", NULL, NULL, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "SlipIdle01", IdleSlipCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Idle01", IdleCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Walk01", WalkCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "Run01", RunAnyCheck, MeleeStopCB,
                            0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "RunOutOfWorld01", RunOutOfWorldCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01 JumpMelee01", "SlipRun01", RunSlipCheck,
                            MeleeStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpMelee01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 Inactive_sleep SlipIdle01 Inactive01 Inactive02 Walk01 Run01 Land01 LandRun01 ",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 5, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Melee01", "SpatulaGrab01", SpatulaGrabCheck,
                            SpatulaMeleeStopCB, 0x00, 0, 0.0, 0.0, 5, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "SlipIdle01",
                            IdleSlipCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Idle01",
                            IdleCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Walk01",
                            WalkCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "Run01",
                            RunAnyCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy",
                            "RunOutOfWorld01", RunOutOfWorldCheck, LassoStopCB, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoEnemyWin LassoEnemyLose LassoDestroy", "SlipRun01",
                            RunSlipCheck, LassoStopCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "DJumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.20, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.08, NULL);
    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 SlipIdle01 Walk01 Run01 RunOutOfWorld01 SlipRun01 Land01 LandHigh01 LandRun01 JumpStart01 JumpApex01 DJumpApex01 Fall01",
        "BounceStart01", BounceCheck, BounceCB, 0, 0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopterHeadUp01 LCopter01", "BounceStart01", BounceCheck,
                            BounceStopLCopterCB, 0x00, 0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 SlipRun01",
        "LassoWindup", LassoStartCheck, LassoStartCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 DJumpApex01 Fall01 BounceStart01 BounceLift01 BounceApex01",
        "JumpMelee01", MeleeCheck, JumpMeleeCB, 0, 0, 0.0, 0.0, 2, 0, 0.06, NULL);
    xAnimTableNewTransition(animTable, "DJumpApex01", "JumpMelee01", MeleeCheck, JumpMeleeCB, 0, 0,
                            0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Land01 Run01 SlipRun01",
        "Melee01", MeleeCheck, MeleeCB, 0, 0, 0.0, 0.0, 2, 0, 0.00, NULL);
    xAnimTableNewTransition(
        animTable,
        "LassoWindup LassoThrow LassoFly LassoDestroy LassoAboutToDestroy LassoEnemyRope LassoEnemyFight LassoEnemyWin LassoEnemyLose",
        "Idle01", LassoLostTargetCheck, LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LassoFly", "LassoDestroy", LassoStraightToDestroyCheck,
                            LassoDestroyCB, 0, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "LassoDestroy", "LassoDestroy", LassoReyankCheck,
                            LassoYankCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoFly", "LassoAboutToDestroy", LassoAboutToDestroyCheck,
                            LassoDestroyCB, 0, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "LassoDestroy", LassoDestroyCheck,
                            LassoYankCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "SlipIdle01", LassoFailIdleSlipCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Idle01", LassoFailIdleCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Walk01", LassoFailWalkCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "Run01", LassoFailRunCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "RunOutOfWorld01",
                            LassoFailRunOutOfWorldCheck, LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable, "LassoAboutToDestroy", "SlipRun01", LassoFailRunSlipCheck,
                            LassoStopCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "SlipIdle01", IdleSlipCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Idle01", IdleCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Walk01", WalkCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Run01", RunAnyCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "RunOutOfWorld01",
                            RunOutOfWorldCheck, StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "SlipRun01", RunSlipCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Walk01 Run01 RunOutOfWorld01 LandRun01 Idle01 SlipRun01",
                            "SlipIdle01", IdleSlipCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "Walk01 Run01 RunOutOfWorld01 LandRun01 SlipIdle01 SlipRun01", "Idle01",
                            IdleCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 RunOutOfWorld01 SlipRun01 LandRun01 Land01 LandHigh01",
        "Walk01", WalkCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep RunOutOfWorld01 SlipRun01 Walk01 Land01 LandHigh01",
        "Run01", RunAnyCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 SlipRun01 Walk01 Land01 LandHigh01",
        "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Run01 RunOutOfWorld01 Walk01 Land01 LandHigh01",
        "SlipRun01", RunSlipCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", JumpApexCheck, NULL, 0, 0, 0.0,
                            0.0, 1, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable,
                            "JumpStart01 JumpApex01 Fall01 BounceStart01 BounceLift01 BounceApex01",
                            "DJumpApex01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.1,
                            NULL);
    xAnimTableNewTransition(animTable, "LCopter01 LCopterHeadUp01", "Fall01", FallCheck,
                            StopLCopterCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 RunOutOfWorld01 Run01 SlipRun01 Land01 LandHigh01",
        "Fall01", FallCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "Land01", LandCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "SlipIdle01",
                            LandSlipIdleCheck, SandyLandCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "LandRun01", LandFastCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "Walk01", LandWalkCheck,
                            SandyLandCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01 DJumpApex01 Fall01", "SlipRun01",
                            LandSlipRunCheck, SandyLandCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Fall01 DJumpApex01 TailSlideFall01 TailSlideDJumpApex01",
                            "LCopterHeadUp01", LCopterCheck, LCopterCB, 0, 0, 0.0, 0.0, 8, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable, "LCopterHeadUp01", "LCopter01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 Fall01 DJumpApex01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01 TailSlideDJumpApex01 LCopter01 LCopterHeadUp01",
        "LedgeGrab01", LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0x0B, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01",
        "TailSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TailSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0x00, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlide01", "TailSlideFall01", TrackFallCheck,
                            TrackFallCB, 0x00, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideDJumpApex01", "TailSlideFall01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0x00, 0, 0.20, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpApex01", "TailSlideFall01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlideLand01", "TailSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01", "TailSlideJumpApex01", NULL, NULL,
                            0x10, 0, 0.0, 0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TailSlide01 TailSlideLand01", "TailSlideJumpStart01",
                            JumpCheck, JumpCB, 0x00, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01",
                            "TailSlideJumpStart01", TrackPrefallJumpCheck, JumpCB, 0x00, 0, 0.0,
                            0.0, 0x0F, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01", "TailSlideJumpApex01", JumpApexCheck,
                            NULL, 0x00, 0, 0.0, 0.0, 0x01, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideFall01",
                            "TailSlideDJumpApex01", DblJumpCheck, DblJumpCB, 0x00, 0, 0.0, 0.0,
                            0x0A, 0, 0.10, NULL);

    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "TailSlideLand01", LandTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "Land01", LandNoTrackCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "LandRun01", LandNoTrackFastCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "Walk01", LandNoTrackWalkCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "SlipIdle01", LandNoTrackSlipIdleCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable, "TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01",
        "SlipRun01", LandNoTrackSlipRunCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat01", Defeated01Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat02", Defeated02Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat03", Defeated03Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "Defeat04", Defeated04Check, DefeatedCB, 0, 4, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive01 Inactive02 Inactive_sleep Walk01 LandRun01 Run01 RunOutOfWorld01 SlipRun01 JumpLift01 JumpApex01 Fall01 Land01 DJumpApex01 FallHigh01 LandHigh01 LCopter01 LCopterHeadUp01 TailSlide01 TailSlideJumpStart01 TailSlideJumpApex01 TailSlideDJumpApex01 TailSlideFall01 TailSlideLand01",
        "DefeatGoo", GooCheck, GooDeathCB, 0, 0, 0.0, 0.0, 0x1e, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle01b Idle01c Idle02 Idle04 Idle05 SlipIdle01 Inactive_sleep Inactive01 Inactive02 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00000,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00001,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00002,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, IdleCB, 0x00, 0x00003,
                            0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle01b", InactiveCheck, InactiveCB, 0x00,
                            0x80000, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle01c", InactiveCheck, InactiveCB, 0x00,
                            0x80001, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle02", InactiveCheck, InactiveCB, 0x00, 0x80002,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle04", InactiveCheck, InactiveCB, 0x00, 0x80003,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle05", InactiveCheck, InactiveCB, 0x00, 0x80004,
                            0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive_sleep", InactiveCheck, InactiveCB, 0x00,
                            0x80005, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0x00,
                            0x80006, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0x00,
                            0x80007, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01b", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01c", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle05", "Idle01", InactiveFinishedCheck, IdleCB, 0x00,
                            0x00000, 0.0, 0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive01", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive02", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0,
                            0.0, 0x00, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit01", "Idle01", NULL, IdleCB, 0x10, 0x00000, 0.0, 0.0,
                            0x00, 0, 0.15, NULL);

    return animTable;
}

xAnimTable* zPatrick_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("Patrick", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive03", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive04", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive05", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive06", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive07", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk02", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk03", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run02", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run03", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunJump", 0x20, 0x400a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunFall", 0x10, 0x400a, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "StunLand", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit02", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit03", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Melee01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatedGoo01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DefeatedProjectile01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "ButtSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Pickup", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_PickupFail", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Idle", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Walk", 0x10, 0x0004, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_Throw", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_PickupItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_IdleItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_WalkItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Carry_ThrowItem", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 Land01 LandHigh01 Melee01 Carry_Idle Carry_Walk Carry_IdleItem Carry_WalkItem",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 5, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Carry_Pickup", "Carry_Idle", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_Throw", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_PickupFail", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "Carry_Idle", "Carry_Walk", AnyMoveCheck, NULL, 0x00, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Carry_Walk", "Carry_Idle", AnyStopCheck, NULL, 0x00, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_Idle Carry_Walk", "Carry_Throw", PatrickGrabThrow,
                            PatrickGrabThrowCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 SlipRun01",
        "Carry_Pickup", PatrickGrabCheck, PatrickGrabCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03",
        "Carry_PickupFail", PatrickGrabFailed, NULL, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_Idle Carry_Walk Carry_Pickup", "Idle01",
                            PatrickGrabKill, NULL, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 SlipRun01",
        "Melee01", PatrickAttackCheck, PatrickMeleeCB, 0, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Melee01", "SlipIdle01", IdleSlipCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Idle01", IdleCheck, NULL, 0x10, 0, 0.0, 0.0, 1,
                            0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk01", WalkStoicCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk02", WalkVictoryCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Walk03", WalkScaredCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run01", RunStoicCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run02", RunVictoryCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "Run03", RunScaredCheck, NULL, 0x10, 0, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0x10,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Melee01", "SlipRun01", RunSlipCheck, NULL, 0x10, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Inactive01 Land01 LandHigh01", "Idle01", NULL, NULL, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.0, NULL);
    xAnimTableNewTransition(animTable, "DJumpStart01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.0, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 SlipIdle01 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01 JumpStart01 JumpApex01 DJumpStart01 Fall01 StunJump StunFall StunLand",
        "BounceStart01", BounceCheck, BounceCB, 0, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable, "Idle01 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "SlipIdle01", IdleSlipCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SlipIdle01 Walk01 Run01 RunOutOfWorld01 SlipRun01",
                            "Idle01", IdleCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Idle01 Walk02 Run02", "Idle02", IdleVictoryCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Run02", "Walk02", WalkVictoryCheck, IdleCB, 0, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Walk02", "Run02", RunVictoryCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02 Walk02 Run02", "Idle01", IdleStoicCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Walk03 Run03 Idle01", "Idle04", IdleScaredCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Run03", "Walk03", WalkScaredCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Walk03", "Run03", RunScaredCheck, IdleCB, 0, 0, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04 Walk03 Run03", "Idle01", IdleStoicCheck, IdleCB, 0,
                            0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk01", WalkStoicCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk02", WalkVictoryCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Walk03", WalkScaredCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Run01", RunStoicCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run03 RunOutOfWorld01 SlipRun01",
        "Run02", RunVictoryCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 RunOutOfWorld01 SlipRun01",
        "Run03", RunScaredCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 Run03 SlipRun01",
        "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Land01 LandHigh01 Run01 Run02 Run03 RunOutOfWorld01",
        "SlipRun01", RunSlipCheck, NULL, 0, 0, 0.0, 0.0, 0x01, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable,
                            "JumpStart01 Fall01 FallHigh01 BounceStart01 BounceLift01 BounceApex01",
                            "DJumpStart01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15,
                            NULL);

    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "Land01",
                            LandCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "SlipRun01",
                            LandSlipRunCheck, NULL, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Fall01 FallHigh01 JumpStart01 DJumpStart01", "SlipIdle01",
                            LandSlipIdleCheck, NULL, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Land01 LandHigh01",
        "Fall01", FallCheck, NULL, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpStart01 Fall01 FallHigh01 DJumpStart01 BounceStart01 BounceLift01 BounceApex01",
        "StunJump", PatrickStunCheck, NULL, 0, 0, 0.0, 0.0, 0x0A, 0, 0.125, NULL);

    xAnimTableNewTransition(animTable, "StunJump", "StunFall", NULL, StunStartFallCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "StunFall", "StunLand", LandCheck, StunRadiusCB, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.1, NULL);
    xAnimTableNewTransition(animTable, "StunLand", "Idle01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.0, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01 Fall01 FallHigh01 DJumpStart01", "LedgeGrab01",
                            LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0x0B, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 Fall01 FallHigh01 Land01 LandHigh01",
        "ButtSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw",
                            "ButtSlide01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0,
                            0.15, NULL);

    xAnimTableNewTransition(animTable, "ButtSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0, 0, 0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "ButtSlide01", "Fall01", TrackFallCheck, TrackFallCB, 0, 0,
                            0.0, 0.0, 0x09, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "ButtSlide01", "JumpStart01", JumpCheck, JumpCB, 0, 0, 0.0,
                            0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01 Fall01 FallHigh01", "JumpStart01",
                            TrackPrefallJumpCheck, JumpCB, 0, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Idle01", "Idle03", InactiveCheck, InactiveCB, 0, 0x080000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0,
                            0x080001, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0,
                            0x080002, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive03", InactiveCheck, InactiveCB, 0,
                            0x080003, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive04", InactiveCheck, InactiveCB, 0,
                            0x080004, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive05", InactiveCheck, InactiveCB, 0,
                            0x080005, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive06", InactiveCheck, InactiveCB, 0,
                            0x080006, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive07", InactiveCheck, InactiveCB, 0,
                            0x080007, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle03", "Idle01", InactiveFinishedCheck, IdleCB, 0,
                            0x000000, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable, "Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Hit01 Hit02 Hit03 Hit04", "Idle01", NULL, IdleCB, 0x10, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);

    xAnimTransition* pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Defeated01", Defeated01Check, DefeatedCB, 0, 2, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "Defeated02", Defeated02Check, DefeatedCB, 0, 2, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    pTran = xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Walk01 Walk02 Walk03 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
        "DefeatedGoo01", GooCheck, GooDeathCB, 0, 0, 0.0, 0.0, 0x10, 0, 0.15, NULL);

    xAnimTableAddTransition(
        animTable, pTran,
        "JumpStart01 Fall01 Land01 DJumpStart01 FallHigh01 LandHigh01 LedgeGrab01 StunJump");
    xAnimTableAddTransition(animTable, pTran,
                            "StunFall StunLand Hit01 Hit02 Hit03 Hit04 Melee01 ButtSlide01");
    xAnimTableAddTransition(
        animTable, pTran,
        "Carry_Pickup Carry_PickupFail Carry_Idle Carry_Walk Carry_Throw Carry_PickupItem");
    xAnimTableAddTransition(animTable, pTran, "Carry_IdleItem Carry_WalkItem Carry_ThrowItem");

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, IdleCB, 0, 0, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, IdleCB, 0, 1, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, IdleCB, 0, 2, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, IdleCB, 0, 3, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);

    return animTable;
}

xAnimTable* zEntPlayer_AnimTable()
{
    static const char* const STANDARD_STATES[33] = {
        "Idle01",     "Walk01",     "Run01",      "Run02",      "Run03",      "RunOutOfWorld01",
        "SlipRun01",  "SlipIdle01", "Land01",     "LandHigh01", "Idle02",     "Idle03",
        "Idle04",     "Idle05",     "Idle06",     "Idle07",     "Idle08",     "Idle09",
        "Idle10",     "Idle11",     "Idle12",     "Idle13",     "Inactive01", "Inactive02",
        "Inactive03", "Inactive04", "Inactive05", "Inactive06", "Inactive07", "Inactive08",
        "Inactive09", "Inactive10", NULL
    };

    static const char* const HIT_STATES[64] = { "Idle01",
                                          "SlipIdle01",
                                          "Walk01",
                                          "Run01",
                                          "Run02",
                                          "Run03",
                                          "SlipRun01",
                                          "Land01",
                                          "LandHigh01",
                                          "LandRun01",
                                          "Idle02",
                                          "Idle03",
                                          "Idle04",
                                          "Idle05",
                                          "Idle06",
                                          "Idle07",
                                          "Idle08",
                                          "Idle09",
                                          "Idle10",
                                          "Idle11",
                                          "Idle12",
                                          "Idle13",
                                          "Inactive01",
                                          "Inactive02",
                                          "Inactive03",
                                          "Inactive04",
                                          "Inactive05",
                                          "Inactive06",
                                          "Inactive07",
                                          "Inactive08",
                                          "Inactive09",
                                          "Inactive10",
                                          "TongueStart01",
                                          "TongueSlide01",
                                          "TongueJump01",
                                          "TongueJumpXtra01",
                                          "TongueDJumpApex01",
                                          "TongueLand01",
                                          "JumpStart01",
                                          "JumpApex01",
                                          "DJumpStart01",
                                          "DJumpLift01",
                                          "Fall01",
                                          "Bspin01",
                                          "BbashStart01",
                                          "BbashAttack01",
                                          "BbashStrike01",
                                          "BbounceStart01",
                                          "BbounceAttack01",
                                          "BbounceStrike01",
                                          "BounceStart01",
                                          "BounceLift01",
                                          "BounceApex01",
                                          "Bbowl01",
                                          "BbowlStart01",
                                          "BbowlWindup01",
                                          "BbowlToss01",
                                          "BbowlRecover01",
                                          "WallLaunch01",
                                          "WallFlight01",
                                          "WallFlight02",
                                          "WallLand01",
                                          "WallFall01",
                                          NULL };

    xAnimTable* animTable = xAnimTableNew("SB", NULL, 0);

    xAnimTableNewState(animTable, "Idle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle05", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle06", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle07", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle08", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle09", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle10", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle11", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle12", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Idle13", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipIdle01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive01", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive02", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive03", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive04", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive05", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive06", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive07", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive08", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive09", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Inactive10", 0x20, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Walk01", 0x10, 0x0044, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run02", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Run03", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "RunOutOfWorld01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SlipRun01", 0x10, 0x0046, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "JumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Fall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Land01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandRun01", 0x20, 0x0006, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BounceApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "DJumpLift01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "FallHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LandHigh01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Bspin01", 0x20, 0x080A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashAttack01", 0x10, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashStrike01", 0x20, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbashMiss01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceAttack01", 0x10, 0x4000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbounceStrike01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Bbowl01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlStart01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlWindup01", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlToss01", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BbowlRecover01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "LedgeGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit02", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit03", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit04", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Hit05", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated01", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated02", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated03", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated04", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Defeated05", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueSlide01", 0x10, 0x1840, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueStart01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJump01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJumpXtra01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueDJumpApex01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueFall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueLand01", 0x20, 0x1800, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueTumble01", 0x20, 0x1800, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Goo01", 0x10, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Goo02", 0x20, 0x0000, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "GooDefeated", 0x00, 0x0480, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallLaunch01", 0x20, 0x008A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFlight01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFlight02", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallLand01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "WallFall01", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BoulderRoll01", 0x20, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "BoulderRoll02", 0x10, 0x000A, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk04", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk03", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk02", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "Talk01", 0x10, 0x0001, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "SpatulaGrab01", 0x20, 0x0080, 1.0, NULL, NULL, 0.0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 Land01 LandRun01",
        "SpatulaGrab01", SpatulaGrabCheck, SpatulaGrabCB, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "SpatulaGrab01", "Idle01", NULL, SpatulaGrabStopCB, 0x10, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle02", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle03", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle04", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle05", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle06", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle07", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle08", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle09", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle11", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle12", "Idle01", InactiveFinishedCheck, IdleCB, 0x00, 0,
                            0.0, 0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive02", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive03", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive04", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive05", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive06", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive07", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive08", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive09", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Inactive10", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Land01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.10, NULL);
    xAnimTableNewTransition(animTable, "LandHigh01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.10, NULL);

    xAnimTableNewTransition(animTable, "Idle01 Walk01 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01",
                            "SlipIdle01", IdleSlipCheck, IdleCB, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "LandRun01", "SlipIdle01", IdleSlipCheck, IdleCB, 0,
                            0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "SlipIdle01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Walk01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run02", "Idle10", IdleVictoryCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run02", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle10", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle10", IdleVictoryCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run03", "Idle13", IdleScaredCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Run03", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle13", "Idle01", IdleStoicCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle13", IdleScaredCheck, IdleCB, 0, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "SlipRun01", "Idle01", IdleCheck, IdleCB, 0, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle02", InactiveCheck, InactiveCB, 0, 0x00140000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle03", InactiveCheck, InactiveCB, 0, 0x00140001,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle04", InactiveCheck, InactiveCB, 0, 0x00140002,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle05", InactiveCheck, InactiveCB, 0, 0x00140003,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle06", InactiveCheck, InactiveCB, 0, 0x00140004,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle07", InactiveCheck, InactiveCB, 0, 0x00140005,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle08", InactiveCheck, InactiveCB, 0, 0x00140006,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle09", InactiveCheck, InactiveCB, 0, 0x00140007,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle11", InactiveCheck, InactiveCB, 0, 0x00140008,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Idle12", InactiveCheck, InactiveCB, 0, 0x00140009,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive01", InactiveCheck, InactiveCB, 0,
                            0x0014000a, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive02", InactiveCheck, InactiveCB, 0,
                            0x0014000b, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive03", InactiveCheck, InactiveCB, 0,
                            0x0014000c, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive04", InactiveCheck, InactiveCB, 0,
                            0x0014000d, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive05", InactiveCheck, InactiveCB, 0,
                            0x0014000e, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive06", InactiveCheck, InactiveCB, 0,
                            0x0014000f, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive07", InactiveCheck, InactiveCB, 0,
                            0x00140010, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive08", InactiveCheck, InactiveCB, 0,
                            0x00140011, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive09", InactiveCheck, InactiveCB, 0,
                            0x00140012, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Idle01", "Inactive10", InactiveCheck, InactiveCB, 0,
                            0x00140013, 0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTransition* tranTbl1[8];

    tranTbl1[0] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Walk01", WalkCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[1] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run01", RunStoicCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[2] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run02", RunVictoryCheck, NoSlipCB,
                                0, 0x00000000, 0.0f, 0.0f, 0x03, 0, 0.15f, NULL);
    tranTbl1[3] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Run03", RunScaredCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x02, 0, 0.15f, NULL);
    tranTbl1[4] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "SlipRun01", RunSlipCheck, SlipRunCB,
                                0, 0x00000000, 0.0f, 0.0f, 0x02, 0, 0.15f, NULL);
    tranTbl1[5] = xAnimTableNewTransition(animTable, STANDARD_STATES[0], "RunOutOfWorld01",
                                          RunOutOfWorldCheck, NoSlipCB, 0, 0x00200100, 0.0f, 0.0f,
                                          0x02, 0, 0.15f, NULL);
    tranTbl1[6] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "Fall01", FallCheck, NoSlipCB, 0,
                                0x00000000, 0.0f, 0.0f, 0x01, 0, 0.15f, NULL);
    tranTbl1[7] =
        xAnimTableNewTransition(animTable, STANDARD_STATES[0], "BbashStart01", BubbleBashCheck,
                                BubbleBashCB, 0, 0x00000000, 0.0f, 0.0f, 0x0A, 0, 0.00f, NULL);

    for (int i = 1; STANDARD_STATES[i] != NULL; i++)
    {
        for (U32 a = 0; a < 8; a++)
        {
            if ((strcmp(STANDARD_STATES[i], tranTbl1[a]->Dest->Name) != 0) &&
                ((i != 5 || ((S32)a != 7))))
            {
                xAnimTableAddTransition(animTable, tranTbl1[a], STANDARD_STATES[i]);
            }
        }
    }

    xAnimTableAddTransition(animTable, tranTbl1[4], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[5], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[6], "LandRun01");
    xAnimTableAddTransition(animTable, tranTbl1[7], "LandRun01");

    xAnimTableNewTransition(animTable, "LandRun01", "Walk01", WalkCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run01", RunStoicCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run02", RunVictoryCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 3, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "Run03", RunScaredCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL,
                            0x10, 0x00200100, 0.0, 0.0, 3, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "LandRun01", "SlipRun01", RunSlipCheck, SlipRunCB, 0x10,
                            0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);

    xAnimTransition* pTran = xAnimTableNewTransition(animTable, "Idle01", "JumpStart01", JumpCheck,
                                                     JumpCB, 0, 0, 0.0, 0.0, 10, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "RunOutOfWorld01");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "Idle02");
    xAnimTableAddTransition(animTable, pTran, "Idle03");
    xAnimTableAddTransition(animTable, pTran, "Idle04");
    xAnimTableAddTransition(animTable, pTran, "Idle05");
    xAnimTableAddTransition(animTable, pTran, "Idle06");
    xAnimTableAddTransition(animTable, pTran, "Idle07");
    xAnimTableAddTransition(animTable, pTran, "Idle08");
    xAnimTableAddTransition(animTable, pTran, "Idle09");
    xAnimTableAddTransition(animTable, pTran, "Idle10");
    xAnimTableAddTransition(animTable, pTran, "Idle11");
    xAnimTableAddTransition(animTable, pTran, "Idle12");
    xAnimTableAddTransition(animTable, pTran, "Idle13");
    xAnimTableAddTransition(animTable, pTran, "Inactive01");
    xAnimTableAddTransition(animTable, pTran, "Inactive02");
    xAnimTableAddTransition(animTable, pTran, "Inactive03");
    xAnimTableAddTransition(animTable, pTran, "Inactive04");
    xAnimTableAddTransition(animTable, pTran, "Inactive05");
    xAnimTableAddTransition(animTable, pTran, "Inactive06");
    xAnimTableAddTransition(animTable, pTran, "Inactive07");
    xAnimTableAddTransition(animTable, pTran, "Inactive08");
    xAnimTableAddTransition(animTable, pTran, "Inactive09");
    xAnimTableAddTransition(animTable, pTran, "Inactive10");
    xAnimTableAddTransition(animTable, pTran, "Goo01");

    pTran = xAnimTableNewTransition(animTable, "Idle01", "BounceStart01", BounceCheck, BounceCB, 0,
                                    0, 0.0, 0.0, 0x0f, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "RunOutOfWorld01");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "JumpStart01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");
    xAnimTableAddTransition(animTable, pTran, "BbounceAttack01");
    xAnimTableAddTransition(animTable, pTran, "BbounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BbashMiss01");

    xAnimTransition* tranTbl2[11];
    tranTbl2[0] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit01", Hit01Check, Hit01CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[1] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit02", Hit02Check, Hit02CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[2] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit03", Hit03Check, Hit03CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[3] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit04", Hit04Check, Hit04CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[4] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Hit05", Hit05Check, Hit05CB, 0,
                                          0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    tranTbl2[5] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated01", Defeated01Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[6] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated02", Defeated02Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[7] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated03", Defeated03Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[8] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated04", Defeated04Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[9] = xAnimTableNewTransition(animTable, HIT_STATES[0], "Defeated05", Defeated05Check,
                                          DefeatedCB, 0, 5, 0.0, 0.0, 0x1e, 0, 0.15, NULL);
    tranTbl2[10] = xAnimTableNewTransition(animTable, HIT_STATES[0], "GooDefeated", GooCheck,
                                           GooDeathCB, 0, 0, 0.0, 0.0, 0x1e, 0, 0.15, NULL);

    for (U32 i = 1; HIT_STATES[i] != NULL; i++)
    {
        for (U32 a = 0; a < 11; a++)
        {
            xAnimTableAddTransition(animTable, tranTbl2[a], HIT_STATES[i]);
        }
    }

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 RunOutOfWorld01 Land01 LandHigh01 LandRun01 SlipIdle01 SlipRun01",
        "TongueStart01", SlideTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueSlide01", "Idle01", NoslideTrackCheck, NoslideTrackCB,
                            0x00, 0, 0.0, 0.0, 9, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TongueStart01", "TongueSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.00, NULL);

    xAnimTableNewTransition(animTable, "TongueSlide01 TongueLand01 TongueStart01", "TongueFall01",
                            TrackFallCheck, TrackFallCB, 0, 0, 0.0, 0.0, 9, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueDJumpApex01", "TongueFall01", NULL, NULL, 0x10, 0,
                            0.0, 0.0, 0x00, 0, 0.20, NULL);
    xAnimTableNewTransition(animTable, "TongueJump01", "TongueFall01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TongueLand01", "TongueSlide01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0x00, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "TongueSlide01 TongueLand01", "TongueJump01", JumpCheck,
                            JumpCB, 0x00, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "TongueJump01 TongueFall01", "TongueJump01",
                            TrackPrefallJumpCheck, JumpCB, 0x00, 0, 0.0, 0.0, 0x0F, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueFall01", "TongueDJumpApex01", DblJumpCheck, DblJumpCB,
                            0, 0, 0.0, 0.0, 0x0A, 0, 0.2, NULL);

    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "TongueLand01", LandTrackCheck, SlideTrackCB, 0, 0, 0.0, 0.0, 1, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "Land01", LandNoTrackCheck, NoslideTrackCB, 0, 0, 0.0, 0.0, 1, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "LandRun01", LandNoTrackFastCheck, NULL, 0, 0, 0.0, 0.0, 3, 0, 0.15,
                            NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "Walk01", LandNoTrackWalkCheck, NULL, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "SlipRun01", LandNoTrackSlipRunCheck, SlipRunCB, 0, 0, 0.0, 0.0, 4, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable,
                            "TongueJump01 TongueJumpXtra01 TongueDJumpApex01 TongueFall01",
                            "SlipIdle01", LandNoTrackSlipIdleCheck, NULL, 0, 0, 0.0, 0.0, 4, 0,
                            0.15, NULL);

    xAnimTableNewTransition(animTable, "TongueJump01", "TongueJumpXtra01", DblJumpCheck,
                            TongueDblJumpCB, 0x20, 0, 0.0, 0.0, 0x0A, 0, 0.00, NULL);

    xAnimTableNewTransition(animTable, "TongueJumpXtra01", "TongueDJumpApex01", NULL,
                            TongueDblSpinCB, 0x10, 0, 0.0, 0.0, 1, 0, 0.0, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 Walk01 Run01 Run02 Run03 RunOutOfWorld01 SlipRun01 SlipIdle01 Land01 LandRun01",
        "BoulderRoll01", BoulderRollCheck, BoulderRollWindupCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.5, NULL);

    xAnimTableNewTransition(animTable, "BoulderRoll01", "BoulderRoll02", NULL, BoulderRollCB, 0x10,
                            0x00000000, 0.0, 0.0, 0x00, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "SlipIdle01", IdleSlipCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Idle01", IdleCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Walk01", WalkCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run01", RunStoicCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run02", RunVictoryCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Run03", RunScaredCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "RunOutOfWorld01", RunOutOfWorldCheck, NULL,
                            0x00, 0x00200100, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "SlipRun01", RunSlipCheck, SlipRunCB, 0x00,
                            0x00000000, 0.0, 0.0, 0x0A, 0, 0.25, NULL);
    xAnimTableNewTransition(animTable, "BoulderRoll02", "Fall01", FallCheck, NULL, 0x00, 0x00000000,
                            0.0, 0.0, 0x0A, 0, 0.25, NULL);

    xAnimTableNewTransition(animTable, "JumpStart01", "JumpApex01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "JumpApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.08, NULL);
    xAnimTableNewTransition(animTable, "BounceStart01", "BounceLift01", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceLift01", "BounceApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BounceApex01", "Fall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "DJumpStart01", "DJumpLift01", NULL, NULL, 0x10, 0, 0.0, 0.0,
                            0, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "JumpLift01", "JumpApex01", JumpApexCheck, NULL, 0x00, 0,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 BounceStart01 BounceLift01 BounceApex01 Fall01",
        "DJumpStart01", DblJumpCheck, DblJumpCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpStart01 JumpLift01 JumpApex01 Fall01 WallFlight01 WallFlight02 WallLand01 WallFall01 DJumpStart01 DJumpLift01 DJumpApex01",
        "WallLaunch01", WallJumpLaunchCheck, WallJumpLaunchCallback, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15,
        NULL);

    xAnimTableNewTransition(animTable, "WallLaunch01", "WallFlight01", NULL, WallJumpCallback, 0x10,
                            0, 0.0, 0.0, 0, 0, 0.0, NULL);
    xAnimTableNewTransition(animTable, "WallFlight01", "WallFlight02", NULL, NULL, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.0, NULL);

    xAnimTableNewTransition(animTable, "WallFlight01 WallFlight02", "WallLand01",
                            WallJumpFlightLandCheck, WallJumpFlightLandCallback, 0x00, 0, 0.0, 0.0,
                            0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "WallLand01", "WallFlight02", WallJumpLandFlightCheck,
                            WallJumpLandFlightCallback, 0x00, 0, 0.0, 0.0, 0, 0, 0.08, NULL);
    xAnimTableNewTransition(animTable, "WallLand01", "WallFall01", NULL, NULL, 0x10, 0, 0.0, 0.0, 0,
                            0, 0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "Land01", LandCheck, LandCallback, 0, 0, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "LandHigh01", LandHighCheck, LandCallback, 0, 0, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "LandRun01", LandRunCheck, LandCallback, 0, 0, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "SlipRun01", LandSlipRunCheck, LandSlipRunCallback, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "JumpApex01 Fall01 DJumpLift01 WallFlight01 WallFlight02 WallLand01 WallFall01 BbashMiss01",
        "SlipIdle01", LandSlipIdleCheck, LandCallback, 0, 0, 0.0, 0.0, 4, 0, 0.15, NULL);

    pTran = xAnimTableNewTransition(animTable, "Idle01", "Bspin01", BubbleSpinCheck, BubbleSpinCB,
                                    0, 0, 0.0, 0.0, 0x0A, 0, 0.05, NULL);

    xAnimTableAddTransition(animTable, pTran, "Idle02");
    xAnimTableAddTransition(animTable, pTran, "Idle03");
    xAnimTableAddTransition(animTable, pTran, "Idle04");
    xAnimTableAddTransition(animTable, pTran, "Idle05");
    xAnimTableAddTransition(animTable, pTran, "Idle06");
    xAnimTableAddTransition(animTable, pTran, "Idle07");
    xAnimTableAddTransition(animTable, pTran, "Idle08");
    xAnimTableAddTransition(animTable, pTran, "Idle09");
    xAnimTableAddTransition(animTable, pTran, "Idle10");
    xAnimTableAddTransition(animTable, pTran, "Idle11");
    xAnimTableAddTransition(animTable, pTran, "Idle12");
    xAnimTableAddTransition(animTable, pTran, "Idle13");
    xAnimTableAddTransition(animTable, pTran, "SlipIdle01");
    xAnimTableAddTransition(animTable, pTran, "Inactive01");
    xAnimTableAddTransition(animTable, pTran, "Inactive02");
    xAnimTableAddTransition(animTable, pTran, "Inactive03");
    xAnimTableAddTransition(animTable, pTran, "Inactive04");
    xAnimTableAddTransition(animTable, pTran, "Inactive05");
    xAnimTableAddTransition(animTable, pTran, "Inactive06");
    xAnimTableAddTransition(animTable, pTran, "Inactive07");
    xAnimTableAddTransition(animTable, pTran, "Inactive09");
    xAnimTableAddTransition(animTable, pTran, "Inactive10");
    xAnimTableAddTransition(animTable, pTran, "Walk01");
    xAnimTableAddTransition(animTable, pTran, "Run01");
    xAnimTableAddTransition(animTable, pTran, "Run02");
    xAnimTableAddTransition(animTable, pTran, "Run03");
    xAnimTableAddTransition(animTable, pTran, "SlipRun01");
    xAnimTableAddTransition(animTable, pTran, "Land01");
    xAnimTableAddTransition(animTable, pTran, "LandHigh01");
    xAnimTableAddTransition(animTable, pTran, "LandRun01");
    xAnimTableAddTransition(animTable, pTran, "JumpStart01");
    xAnimTableAddTransition(animTable, pTran, "JumpLift01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "BounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BounceLift01");
    xAnimTableAddTransition(animTable, pTran, "BounceApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");

    pTran = xAnimTableNewTransition(animTable, "JumpStart01", "BbounceStart01", BubbleBounceCheck,
                                    BubbleBounceCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.15, NULL);

    xAnimTableAddTransition(animTable, pTran, "JumpLift01");
    xAnimTableAddTransition(animTable, pTran, "JumpApex01");
    xAnimTableAddTransition(animTable, pTran, "BounceStart01");
    xAnimTableAddTransition(animTable, pTran, "BounceLift01");
    xAnimTableAddTransition(animTable, pTran, "BounceApex01");
    xAnimTableAddTransition(animTable, pTran, "DJumpStart01");
    xAnimTableAddTransition(animTable, pTran, "DJumpLift01");
    xAnimTableAddTransition(animTable, pTran, "Fall01");
    xAnimTableAddTransition(animTable, pTran, "BbashMiss01");

    xAnimTableNewTransition(animTable, "Bspin01", "SlipIdle01", IdleSlipCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Idle01", IdleCheck, IdleCB, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Walk01", WalkCheck, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run01", RunStoicCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run03", RunScaredCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Run02", RunVictoryCheck, NULL, 0x10, 0x00000000,
                            0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "RunOutOfWorld01", RunOutOfWorldCheck, NULL, 0x10,
                            0x00200100, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "SlipRun01", RunSlipCheck, SlipRunCB, 0x10,
                            0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Bspin01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0, 0.0,
                            1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbashStart01", "BbashAttack01", NULL, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BbashStart01", "BbashStrike01", BBashStrikeCheck,
                            BBashStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashAttack01", "BbashStrike01", BBashStrikeCheck,
                            BBashStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashAttack01", "BbashMiss01", BBashToJumpCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbashStrike01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbashMiss01", "Fall01", NULL, NULL, 0x10, 0x00000000, 0.0,
                            0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStart01", "BbounceAttack01", NULL, BBounceAttackCB,
                            0x10, 0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbounceAttack01", "BbounceStrike01", BBounceStrikeCheck,
                            BBounceStrikeCB, 0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.00, NULL);
    xAnimTableNewTransition(animTable, "BbounceAttack01", "JumpLift01", BBounceToJumpCheck,
                            BBounceToJumpCB, 0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.10, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "SlipIdle01", IdleSlipCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Idle01", IdleCheck, IdleCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Walk01", WalkCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run01", RunStoicCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run03", RunScaredCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "Run02", RunVictoryCheck, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "RunOutOfWorld01", RunOutOfWorldCheck,
                            NULL, 0x10, 0x00200100, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbounceStrike01", "SlipRun01", RunSlipCheck, SlipRunCB,
                            0x10, 0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable,
                            "JumpStart01 JumpLift01 JumpApex01 Fall01 DJumpStart01 DJumpLift01",
                            "LedgeGrab01", LedgeGrabCheck, LedgeGrabCB, 0, 0, 0.0, 0.0, 0xb, 0, 0.1,
                            NULL);

    xAnimTableNewTransition(animTable, "LedgeGrab01", "Idle01", NULL, LedgeFinishCB, 0x10, 0, 0.0,
                            0.0, 0, 0, 0.1, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive09 Inactive10 Walk01 Land01 LandRun01 Run01 Run02 Run03 SlipRun01",
        "BbowlStart01", BbowlCheck, BbowlCB, 0, 0, 0.0, 0.0, 0x0A, 0, 0.1, NULL);

    xAnimTableNewTransition(animTable, "BbowlStart01", "BbowlWindup01", NULL, NULL, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlWindup01", "BbowlToss01", BbowlWindupEndCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlToss01", "BbowlRecover01", NULL, BbowlTossEndCB, 0x10,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Walk01", BbowlRecoverWalkCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run01", BbowlRecoverRunCheck, NULL, 0x00,
                            0x00000000, 0.0, 0.0, 1, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run03", BbowlRecoverRunScaredCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 2, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Run02", BbowlRecoverRunVictoryCheck, NULL,
                            0x00, 0x00000000, 0.0, 0.0, 3, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "RunOutOfWorld01",
                            BbowlRecoverRunOutOfWorldCheck, NULL, 0x00, 0x00200100, 0.0, 0.0, 3, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "SlipRun01", BbowlRecoverRunSlipCheck,
                            SlipRunCB, 0x00, 0x00000000, 0.0, 0.0, 4, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "BbowlRecover01", "Idle01", NULL, IdleCB, 0x10, 0x00000000,
                            0.0, 0.0, 1, 0, 0.15, NULL);

    bungee_state::insert_animations(*animTable);
    cruise_bubble::insert_player_animations(*animTable);

    xAnimTableNewTransition(animTable, "Hit01", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit02", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit03", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit04", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);
    xAnimTableNewTransition(animTable, "Hit05", "Idle01", NULL, IdleCB, 0x10, 0, 0.0, 0.0, 0, 0,
                            0.15, NULL);

    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk01", TalkCheck, NULL, 0, 0, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk02", TalkCheck, NULL, 0, 1, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk03", TalkCheck, NULL, 0, 2, 0.0, 0.0, 0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(
        animTable,
        "Idle01 Idle02 Idle03 Idle04 Idle05 Idle06 Idle07 Idle08 Idle09 Idle10 Idle11 Idle12 Idle13 SlipIdle01 Inactive01 Inactive02 Inactive03 Inactive04 Inactive05 Inactive06 Inactive07 Inactive08 Inactive09 Inactive10 ",
        "Talk04", TalkCheck, NULL, 0, 3, 0.0, 0.0, 0x14, 0, 0.15, NULL);

    xAnimTableNewTransition(animTable, "Talk01", "Idle01", TalkDoneCheck, NULL, 0, 0, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk02", "Idle01", TalkDoneCheck, NULL, 0, 1, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk03", "Idle01", TalkDoneCheck, NULL, 0, 2, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);
    xAnimTableNewTransition(animTable, "Talk04", "Idle01", TalkDoneCheck, NULL, 0, 3, 0.0, 0.0,
                            0x14, 0, 0.15, NULL);

    return animTable;
}

xAnimTable* zSpongeBobTongue_AnimTable()
{
    xAnimTable* animTable = xAnimTableNew("SBTongue", NULL, 0);

    xAnimTableNewState(animTable, "TongueSlide01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueStart01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJump01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueJumpXtra01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueDJumpApex01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueFall01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(animTable, "TongueLand01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return animTable;
}

xAnimTable* zEntPlayer_BoulderVehicleAnimTable()
{
    xAnimTable* table = xAnimTableNew("BoulderVehicleTable", NULL, 0x0);

    xAnimTableNewState(table, "Idle01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Move01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(table, "Idle01", "Move01", BoulderRollMoveCheck, NULL, 0x0, 0x0, 0.0f,
                            0.0f, 1, 0, 0.45f, NULL);
    xAnimTableNewTransition(table, "Move01", "Idle01", BoulderRollIdleCheck, NULL, 0x0, 0x0, 0.0f,
                            0.0f, 1, 0, 0.45f, NULL);

    return table;
}

xAnimTable* zEntPlayer_TreeDomeSBAnimTable()
{
    xAnimTable* table = xAnimTableNew("SB", NULL, 0x0);

    xAnimTableNewState(table, "Idle01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle02", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle03", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle04", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle05", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle06", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle07", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle08", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle09", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle10", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle11", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle12", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Idle13", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SlipIdle01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive01", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive02", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive03", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive04", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive05", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive06", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive07", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive08", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive09", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Inactive10", 0x20, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Walk01", 0x10, 0x44, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run02", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Run03", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "RunOutOfWorld01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SlipRun01", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "JumpApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Fall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Land01", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LandRun01", 0x20, 0x6, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BounceApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "DJumpStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "DJumpLift01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "FallHigh01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LandHigh01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Bspin01", 0x20, 0x80A, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashAttack01", 0x10, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashStrike01", 0x20, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbashMiss01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceAttack01", 0x10, 0x4000, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbounceStrike01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Bbowl01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlStart01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlWindup01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlToss01", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BbowlRecover01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "LedgeGrab01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit02", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit03", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit04", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Hit05", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated01", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated02", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated03", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated04", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Defeated05", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueSlide01", 0x10, 0x1840, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueStart01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueJump01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueJumpXtra01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueDJumpApex01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueFall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueLand01", 0x20, 0x1800, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "TongueTumble01", 0x20, 0x1800, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Goo01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Goo02", 0x20, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "GooDefeated", 0x0, 0x480, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallLaunch01", 0x20, 0x8A, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFlight01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFlight02", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallLand01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "WallFall01", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BoulderRoll01", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "BoulderRoll02", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk04", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk03", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk02", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Talk01", 0x10, 0x1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "SpatulaGrab01", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return table;
}
