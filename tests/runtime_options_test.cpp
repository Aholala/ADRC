#include "ALG_ADRC.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();

    void Check(bool value, const char* message)
    {
        if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
    }

    void Near(float actual, float expected, float tolerance, const char* message)
    {
        Check(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance, message);
    }

    bool InitTD(ADRC::LTD2& td, float rate = 0) { return td.Init(20, 0, rate); }
    bool InitTD(ADRC::TD2& td, float rate = 0) { return td.Init(100, .01f, 0, rate); }
    bool RetuneTD(ADRC::LTD2& td, bool valid) { return td.SetParameters(valid ? 30 : nan); }
    bool RetuneTD(ADRC::TD2& td, bool valid) { return td.SetParameters(valid ? 200 : nan, .02f); }

    template<class TD> void TestTD()
    {
        TD td;
        Check(!RetuneTD(td, true), "TD retune requires Init");
        Check(td.SetLimits(.2f, .4f) && InitTD(td), "TD configure limits before Init");
        for (int i = 0; i < 6000; ++i)
        {
            const float target = i < 2000 ? 100.f : -100.f;
            const float x = td.GetOutput(), v = td.GetRate();
            float a = 0;
            Check(td.Calc(target, a) && std::fabs(a) <= .4f, "TD acceleration command bound");
            if (i == 1000)
            {
                Check(RetuneTD(td, true), "TD retune");
                Check(td.GetOutput() == x && td.GetRate() == v, "TD retune preserves states");
            }
            Check(td.Update(target, .001f), "TD constrained step");
            Check(std::fabs(td.GetRate()) <= .2f, "TD speed bound");
            Check(std::fabs(td.GetRate() - v) <= .000401f, "TD effective acceleration bound");
            Near(td.GetOutput(), x + .001f * v, 1e-7f, "TD integrates bounded old velocity");
        }
        const TD before = td;
        Check(!td.SetLimits(.1f, .4f), "TD reject shrinking speed below current state");
        Check(!td.SetLimits(nan, 1) && !td.SetLimits(1, -1) && !td.SetLimits(-inf, 1),
              "TD reject invalid limits");
        Check(!InitTD(td, .3f) && !td.Reset(1, .3f), "TD initial/reset speed must satisfy limit");
        Check(!RetuneTD(td, false), "TD reject invalid retune");
        TD reference = before;
        Check(td.Update(1, .001f) && reference.Update(1, .001f), "TD rollback comparison");
        Check(td.GetOutput() == reference.GetOutput() && td.GetRate() == reference.GetRate(),
              "TD failed operations preserve parameters, states and limits");
        float a = 0;
        Check(td.SetLimits(inf, inf) && td.Calc(100,a) && std::fabs(a) > .4f,
              "TD disable acceleration clamp");
        Check(td.Reset(0,0) && td.SetLimits(0,.4f) && td.Update(100,.001f), "TD zero speed cap");
        Check(td.GetOutput() == 0 && td.GetRate() == 0, "TD zero speed freezes resting trajectory");
        Check(td.SetLimits(.2f,0) && td.Update(100,.001f), "TD zero acceleration cap");
        Check(td.GetOutput() == 0 && td.GetRate() == 0, "TD zero acceleration freezes resting velocity");
    }

    void TestManualObservers()
    {
        ADRC::LESO2 a;
        ADRC::LESO3 b;
        Check(a.Init(ADRC::LESO2Config{2, 3, 2}, .5f, -.25f), "LESO2 manual Init");
        Check(b.Init(ADRC::LESO3Config{2, 6, 11, 6}, .5f, -.25f, .75f), "LESO3 manual Init");
        Check(a.Update(.125f, .25f, .25f) && b.Update(.125f, .25f, .25f), "manual hand step");
        Near(a.GetOutput(), .3125f, 0, "LESO2 old state hand result");
        Near(a.GetDisturbance(), -.375f, 0, "LESO2 manual disturbance hand result");
        Near(b.GetOutput(), .0625f, 0, "LESO3 output hand result");
        Near(b.GetRate(), -.6875f, 0, "LESO3 rate hand result");
        Near(b.GetDisturbance(), .375f, 0, "LESO3 disturbance hand result");
        Check(a.Update(0, 0, 1) && b.Update(0, 0, 1), "caller chooses positive finite period");
        Near(a.GetOutput(),-1,0,"LESO2 large period hand value");
        Near(a.GetDisturbance(),-1,0,"LESO2 large period disturbance");
        Near(b.GetOutput(),-1,0,"LESO3 large period hand value");
        Near(b.GetRate(),-1,0,"LESO3 large period rate");
        Near(b.GetDisturbance(),0,0,"LESO3 large period disturbance");
        Check(b.SetParameters(ADRC::LESO3Config{1, 1, 1, 1}), "gain stability assessed offline");
        Check(!a.SetParameters(ADRC::LESO2Config{1, -1, 1}), "reject negative gains");
        Check(a.Init(ADRC::LESO2Config{1, .2f, 1}), "LESO2 complex pair Init");
        Check(b.Init(ADRC::LESO3Config{1, 1.2f, 1.2f, 1}), "LESO3 complex pair Init");
        Check(a.Update(0, 1, .1f) && b.Update(0, 1, .1f), "complex poles stable Euler step");
        Check(a.Update(0, 1, .3f) && b.Update(0, 1, .3f), "no per-step pole validation");
        ADRC::LESO2 c;
        ADRC::LESO3 d;
        Check(a.Init(2, 8) && c.Init(ADRC::LESO2Config{2, 16, 64}), "LESO2 equivalent gains");
        Check(b.Init(2, 8) && d.Init(ADRC::LESO3Config{2, 24, 192, 512}), "LESO3 equivalent gains");
        for (int i = 0; i < 2048; ++i)
        {
            const float y = std::sin(i * .01f), u = i % 7 * .02f;
            Check(a.Update(u, y, 1.f/512) && c.Update(u, y, 1.f/512) &&
                  b.Update(u, y, 1.f/512) && d.Update(u, y, 1.f/512), "manual/bandwidth step");
            Check(a.GetOutput() == c.GetOutput() && a.GetDisturbance() == c.GetDisturbance() &&
                  b.GetOutput() == d.GetOutput() && b.GetRate() == d.GetRate() &&
                  b.GetDisturbance() == d.GetDisturbance(), "manual/bandwidth exact equivalence");
        }
        const float a_y=a.GetOutput(), a_f=a.GetDisturbance();
        const float b_y=b.GetOutput(), b_v=b.GetRate(), b_f=b.GetDisturbance();
        Check(a.SetParameters(ADRC::LESO2Config{-3,3,2}) &&
              b.SetParameters(ADRC::LESO3Config{-3,6,11,6}), "standalone manual retune");
        Check(a.GetOutput() == a_y && a.GetDisturbance() == a_f &&
              b.GetOutput() == b_y && b.GetRate() == b_v && b.GetDisturbance() == b_f,
              "standalone manual retune preserves all states");

    }

    void TestPeriodOwnership()
    {
        ADRC::LTD2 linear_td;
        ADRC::TD2 td;
        ADRC::LESO2 eso2;
        ADRC::LESO3 eso3;
        ADRC::RLESO1 reduced;
        Check(linear_td.Init(2,1,.5f) && linear_td.Update(1,1), "LTD2 accepts caller period");
        Near(linear_td.GetOutput(),1.5f,0,"LTD2 large period position hand value");
        Near(linear_td.GetRate(),-1.5f,0,"LTD2 large period rate hand value");
        Check(td.Init(2,.5f,.125f,.25f) && td.Update(0,1), "TD2 accepts caller period");
        Near(td.GetOutput(),.375f,0,"TD2 large period position hand value");
        Near(td.GetRate(),-1.25f,0,"TD2 large period rate hand value");
        Check(eso2.Init(2,2,1,.5f) && eso2.Update(0,1,1), "LESO2 bandwidth period is caller-owned");
        Near(eso2.GetOutput(),1.5f,0,"LESO2 large period output hand value");
        Near(eso2.GetDisturbance(),.5f,0,"LESO2 large period disturbance hand value");
        Check(eso3.Init(2,2,1,.5f,.25f) && eso3.Update(0,1,1), "LESO3 bandwidth period is caller-owned");
        Near(eso3.GetOutput(),1.5f,0,"LESO3 large period output hand value");
        Near(eso3.GetRate(),.75f,0,"LESO3 large period rate hand value");
        Check(reduced.Init(2,2,1,.5f) && reduced.Update(0,1,1), "RLESO1 accepts caller period");
        float f=0;
        Check(reduced.Calc(1,f), "RLESO1 reconstruction after caller period");
        Near(f,-.5f,0,"RLESO1 large period hand value");
    }

    bool InitObserver(ADRC::LESO2& o, float f) { return o.Init(2, 10, 0, f); }
    bool InitObserver(ADRC::LESO3& o, float f) { return o.Init(2, 10, 0, 0, f); }
    bool InitObserver(ADRC::ESO2& o, float f) { return o.Init({2,20,100,1,.1f},0,f); }
    bool InitObserver(ADRC::ESO3& o, float f) { return o.Init({2,30,300,1000,1,1,.1f},0,0,f); }
    bool RetuneObserver(ADRC::LESO2& o, bool valid) { return o.SetParameters(valid ? 3 : nan, 20); }
    bool RetuneObserver(ADRC::LESO3& o, bool valid) { return o.SetParameters(valid ? 3 : nan, 20); }
    bool RetuneObserver(ADRC::ESO2& o, bool valid) { return o.SetParameters({valid ? 3.f : nan,40,400,1,.1f}); }
    bool RetuneObserver(ADRC::ESO3& o, bool valid) { return o.SetParameters({valid ? 3.f : nan,60,1200,8000,1,1,.1f}); }
    bool ResetObserver(ADRC::LESO2& o, float f) { return o.Reset(0,f); }
    bool ResetObserver(ADRC::LESO3& o, float f) { return o.Reset(0,0,f); }
    bool ResetObserver(ADRC::ESO2& o, float f) { return o.Reset(0,f); }
    bool ResetObserver(ADRC::ESO3& o, float f) { return o.Reset(0,0,f); }
    float StateRate(const ADRC::LESO2&) { return 0; }
    float StateRate(const ADRC::ESO2&) { return 0; }
    float StateRate(const ADRC::LESO3& o) { return o.GetRate(); }
    float StateRate(const ADRC::ESO3& o) { return o.GetRate(); }

    template<class Observer> void TestObserver()
    {
        Observer o;
        Check(!RetuneObserver(o, true), "observer retune requires Init");
        Check(InitObserver(o, 2), "observer Init");
        Check(o.SetDisturbanceLimit(.25f) && o.GetDisturbance() == 2,
              "limit setter preserves existing full-order state");
        Check(RetuneObserver(o, true) && o.GetDisturbance() == 2 && o.GetOutput() == 0,
              "retune preserves states even after limit tightening");
        Observer reference = o;
        Check(!RetuneObserver(o, false) && !o.SetDisturbanceLimit(nan) &&
              !o.SetDisturbanceLimit(-1) && !o.SetDisturbanceLimit(-inf), "observer invalid options");
        Check(o.Update(0, 0, .001f) && reference.Update(0, 0, .001f), "observer post-retune update");
        Check(o.GetDisturbance() == .25f && o.GetOutput() == reference.GetOutput(),
              "limit effective on next successful update, failed retune preserves gain");
        Check(ResetObserver(o,-2) && o.GetDisturbance() == -.25f, "reset negative disturbance clamp");
        Check(InitObserver(o, 5) && o.GetDisturbance() == .25f, "Init retains configured clamp");
        Check(o.SetDisturbanceLimit(0) && o.Update(0, 1, .001f) && o.GetDisturbance() == 0,
              "zero disturbance limit");
        Check(o.SetDisturbanceLimit(inf) && ResetObserver(o, 10) && o.GetDisturbance() == 10,
              "disable disturbance clamp");
        Check(o.Update(.125f,.5f,.001f), "observer nonzero dynamic states");
        const float output = o.GetOutput(), disturbance = o.GetDisturbance(), rate = StateRate(o);
        Check(RetuneObserver(o,true), "observer dynamic retune");
        Check(o.GetOutput() == output && o.GetDisturbance() == disturbance && StateRate(o) == rate,
              "all observer states preserved at nonzero dynamic operating point");
        for (float bad : {nan,inf,-inf})
        {
            Check(!o.Update(bad,0,.001f) && !o.Update(0,bad,.001f), "nonfinite observation rejected at commit");
            Check(o.GetOutput() == output && o.GetDisturbance() == disturbance && StateRate(o) == rate,
                  "nonfinite observation cannot corrupt any state");
        }
    }

    void TestReduced()
    {
        ADRC::RLESO1 o;
        float f = 0;
        Check(!o.SetParameters(2, 10, 1), "reduced retune requires Init");
        Check(o.Init(2,10,.5f,2) && o.SetParameters(3,40,.5f) && o.Calc(.5f,f), "reduced retune");
        Near(f,2,0,"reduced preserves physical estimate rather than q");
        const float q = o.GetAuxiliaryState();
        Check(!o.SetParameters(0,20,.5f) && !o.SetParameters(1,20,nan), "invalid reduced retune");
        Check(o.GetAuxiliaryState() == q && o.Calc(.5f,f) && f == 2, "reduced rollback");
        Check(o.SetDisturbanceLimit(.25f) && o.GetAuxiliaryState() == q && o.Calc(.5f,f),
              "reduced limit preserves auxiliary state");
        Check(f == .25f && std::fabs(q) > .25f, "reduced bound applies to reconstructed disturbance");
        Check(o.Update(1,10,.001f) && o.Calc(10,f) && std::fabs(f) <= .25f,
              "reduced update projection and returned estimate bound");
        Check(o.SetParameters(3,20,10) && o.Calc(10,f), "reduced retune keeps bound");
        Near(f, .25f, 2e-5f, "reduced bounded estimate remains continuous at retune measurement");
        Check(o.Reset(1,-4) && o.Calc(1,f) && f == -.25f, "reduced reset clamp");
        Check(o.Init(2,10,1,4) && o.Calc(1,f) && f == .25f, "reduced Init retains clamp");
        Check(!o.SetDisturbanceLimit(nan) && !o.SetDisturbanceLimit(-1), "reduced invalid limit");
        Check(o.SetDisturbanceLimit(0) && o.Calc(-100,f) && f == 0, "reduced zero bound");
        Check(o.SetDisturbanceLimit(inf) && o.Reset(1,4) && o.Calc(1,f) && f == 4,
              "reduced disable clamp");
    }

    void TestControllers()
    {
        ADRC::LADRC1 a;
        ADRC::LADRC2 b;
        Check(!a.SetParameters(2,3,10) && !b.SetParameters(2,3,10), "controller retune requires Init");
        Check(a.Init(2,3,10,.5f,.25f) && b.Init(2,3,10,.5f,.25f,.75f), "controller initial states");
        Check(a.SetDisturbanceLimit(.1f) && b.SetDisturbanceLimit(.1f), "controller clamp setters");
        Check(a.SetParameters(3,4,20) && b.SetParameters(3,4,20), "controller bandwidth retune");
        Check(a.GetEstimatedOutput() == .5f && a.GetEstimatedDisturbance() == .25f &&
              b.GetEstimatedOutput() == .5f && b.GetEstimatedRate() == .25f &&
              b.GetEstimatedDisturbance() == .75f, "controller retune preserves all estimates");
        Check(a.SetParameters(ADRC::LESO2Config{4,3,2},5) &&
              b.SetParameters(ADRC::LESO3Config{4,6,11,6},5), "controller manual retune");
        float u = 0, v = 0;
        Check(a.Calc(1,u) && b.Calc(1,.5f,v), "retuned controller requests");
        Near(u, (5.f*.5f-.25f)/4, 0, "LADRC1 feedback and observer b0 agree");
        Near(v, (25.f*.5f+10.f*.25f-.75f)/4, 0, "LADRC2 manual retune feedback");
        Check(!a.SetParameters(0,6,30) && !b.SetParameters(0,6,30) &&
              !a.SetParameters(ADRC::LESO2Config{5,1,1},nan) &&
              !b.SetParameters(ADRC::LESO3Config{5,1,1,-2},6), "controller retune rollback");
        float u_after=0, v_after=0;
        Check(a.Calc(1,u_after) && b.Calc(1,.5f,v_after) && u == u_after && v == v_after,
              "failed controller retune preserves feedback and observer configuration");
        Check(a.UpdateObserver(0,0,.1f) && b.UpdateObserver(0,0,.1f), "controller manual observers step");
        Check(std::fabs(a.GetEstimatedDisturbance()) <= .1f &&
              std::fabs(b.GetEstimatedDisturbance()) <= .1f, "controller forwards disturbance clamp");
        Check(a.UpdateObserver(0,0,1) && b.UpdateObserver(0,0,1), "controller forwards caller period");
        Check(a.Init(ADRC::LESO2Config{2,3,2},4,0,5) &&
              b.Init(ADRC::LESO3Config{2,6,11,6},4,0,0,-5), "controller manual Init");
        Check(a.GetEstimatedDisturbance() == .1f && b.GetEstimatedDisturbance() == -.1f,
              "controller Init retains clamp");
        Check(a.SetParameters(2,4,10) && b.SetParameters(2,4,10), "return to bandwidth configuration");
        Check(a.SetDisturbanceLimit(inf) && b.SetDisturbanceLimit(inf) &&
              a.Reset(0,5) && b.Reset(0,0,5), "controller disable clamp and Reset");
        Check(a.GetEstimatedDisturbance() == 5 && b.GetEstimatedDisturbance() == 5,
              "controller reset exposes unclamped state");
    }

    void TestFeedback()
    {
        ADRC::NLESF1 a;
        ADRC::NLESF2 b;
        Check(!a.SetParameters(2,3,1,.1f) && !b.SetParameters(2,3,1,4,1,.1f),
              "feedback retune requires Init");
        Check(a.Init(1,1,1,.1f) && b.Init(1,1,1,1,1,.1f), "feedback Init");
        Check(a.SetParameters(2,3,1,.1f) && b.SetParameters(2,3,1,4,1,.1f), "feedback retune");
        float u=0, v=0;
        Check(a.Calc(2,1,1,u) && b.Calc(2,1,1,.5f,1,v), "feedback retuned calculation");
        Near(u,1,0,"NLESF1 retuned hand value");
        Near(v,2,0,"NLESF2 retuned hand value");
        Check(!a.SetParameters(0,4,1,.1f) && !b.SetParameters(0,4,1,5,1,.1f), "feedback rollback");
        Check(a.Calc(2,1,1,u) && b.Calc(2,1,1,.5f,1,v) && u == 1 && v == 2,
              "feedback keeps valid parameters after failure");
        for (float bad : {nan,inf,-inf})
        {
            for (int field=0;field<5;++field)
            {
                float args[5]={2,1,1,.5f,1}; args[field]=bad; v=123;
                Check(!b.Calc(args[0],args[1],args[2],args[3],args[4],v) && v == 123,
                      "NLESF2 final result guard rejects each nonfinite input");
            }
            for (int field=0;field<3;++field)
            {
                float args[3]={2,1,1}; args[field]=bad; u=123;
                Check(!a.Calc(args[0],args[1],args[2],u) && u == 123,
                      "NLESF1 final result guard rejects each nonfinite input");
            }
        }
    }
}

int main()
{
    TestTD<ADRC::LTD2>();
    TestTD<ADRC::TD2>();
    TestManualObservers();
    TestPeriodOwnership();
    TestObserver<ADRC::LESO2>();
    TestObserver<ADRC::LESO3>();
    TestObserver<ADRC::ESO2>();
    TestObserver<ADRC::ESO3>();
    TestReduced();
    TestControllers();
    TestFeedback();
    std::cout << "Runtime options: all 11 components passed\n";
}
