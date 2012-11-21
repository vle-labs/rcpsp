/**
 * @file StepScheduler.cpp
 * @author The VLE Development Team
 * See the AUTHORS or Authors.txt file
 */

/*
 * Copyright (C) 2012 ULCO http://www.univ-littoral.fr
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include <vle/devs/Dynamics.hpp>

#include <Activities.hpp>

namespace rcpsp {

class StepScheduler : public vle::devs::Dynamics
{
public:
    StepScheduler(const vle::devs::DynamicsInit& init,
                  const vle::devs::InitEventList& events) :
        vle::devs::Dynamics(init, events)
    {
        mLocation = vle::value::toString(events.get("location"));
    }

    vle::devs::Time init(const vle::devs::Time& /* time */)
    {
        mRunningActivity = 0;
        mPhase = WAIT;
        return vle::devs::Time::infinity;
    }

    void output(const vle::devs::Time& /* time */,
                vle::devs::ExternalEventList& output) const
    {
        if (mPhase == SEND_DEMAND) {
            if (not mWaitingActivities.empty()) {
                vle::devs::ExternalEvent* ee =
                    new vle::devs::ExternalEvent("demand");

                std::cout << "[" << getModel().getParentName()
                          << ":" << getModelName()
                          << "] - " << mWaitingActivities.front()->name()
                          << " - demand" << std::endl;

                ee << vle::devs::attribute("resources",
                    mWaitingActivities.front()->resourceConstraints().
                                           toValue());
                output.addEvent(ee);
            }
        } else if (mPhase == SEND_PROCESS) {
            if (mRunningActivity)
            {
                vle::devs::ExternalEvent* ee =
                    new vle::devs::ExternalEvent("process");

                ee << vle::devs::attribute("activity",
                                           mRunningActivity->toValue());
                output.addEvent(ee);
            }
        } else if (mPhase == SEND_SCHEDULE) {
            for(Activities::const_iterator it = mSchedulingActivities.begin();
                it != mSchedulingActivities.end(); ++it) {
                {
                    vle::devs::ExternalEvent* ee =
                        new vle::devs::ExternalEvent("schedule");

                    ee << vle::devs::attribute("location",
                                               (*it)->location().name());
                    ee << vle::devs::attribute("activity", (*it)->toValue());
                    output.addEvent(ee);
                }
                {
                    vle::devs::ExternalEvent* ee =
                        new vle::devs::ExternalEvent("release");

                    ee << vle::devs::attribute(
                        "resources", (*it)->allocatedResources()->toValue());
                    output.addEvent(ee);
                }
            }
        } else if (mPhase == SEND_DONE) {
            for(Activities::const_iterator it = mDoneActivities.begin();
                it != mDoneActivities.end(); ++it) {

                std::cout << "[" << getModel().getParentName()
                          << ":" << getModelName()
                          << "] - " << (*it)->name()
                          << " - done" << std::endl;

                {
                    vle::devs::ExternalEvent* ee =
                        new vle::devs::ExternalEvent("done");

                    ee << vle::devs::attribute("activity", (*it)->toValue());
                    output.addEvent(ee);
                }
                {
                    vle::devs::ExternalEvent* ee =
                        new vle::devs::ExternalEvent("release");

                    ee << vle::devs::attribute(
                        "resources", (*it)->allocatedResources()->toValue());
                    output.addEvent(ee);
                }
            }
        }
    }

    vle::devs::Time timeAdvance() const
    {
        if (mPhase == SEND_DEMAND or mPhase == SEND_DONE or
            mPhase == SEND_PROCESS or mPhase == SEND_SCHEDULE) {
            return 0;
        } else {
            return vle::devs::Time::infinity;
        }
    }

    void internalTransition(const vle::devs::Time& time)
    {
        if (mPhase == SEND_DEMAND) {
            mPhase = WAIT;
        } else if (mPhase == SEND_DONE) {
            mDoneActivities.clear();
            if (mWaitingActivities.empty()) {
                mPhase = WAIT;
            } else {
                mPhase = SEND_DEMAND;
            }
        } else if (mPhase == SEND_PROCESS) {
            if (mRunningActivity) {
                delete mRunningActivity;
                mRunningActivity = 0;
            }
            mPhase = WAIT;
        } else if (mPhase == SEND_SCHEDULE) {
            mSchedulingActivities.clear();
            if (mWaitingActivities.empty()) {
                mPhase = WAIT;
            } else {
                mPhase = SEND_DEMAND;
            }
        }
    }

    void externalTransition(
        const vle::devs::ExternalEventList& events,
        const vle::devs::Time& time)
    {
        vle::devs::ExternalEventList::const_iterator it = events.begin();

        while (it != events.end()) {
            if ((*it)->onPort("schedule")) {
                if ((*it)->getStringAttributeValue("location") == mLocation) {
                    Activity* a =
                        Activity::build((*it)->getAttributeValue("activity"));

                    std::cout << "[" << getModel().getParentName()
                              << ":" << getModelName()
                              << "] - schedule = " << a->current()->name()
                              << "/" << a->name() << std::endl;

                    mWaitingActivities.push_back(a);
                    mPhase = SEND_DEMAND;
                }
            } else if ((*it)->onPort("assign")) {
                Resources* r = Resources::build(
                    (*it)->getAttributeValue("resources"));

                mRunningActivity = mWaitingActivities.front();
                mRunningActivity->assign(r);
                mWaitingActivities.pop_front();
                mPhase = SEND_PROCESS;
            } else if ((*it)->onPort("done")) {
                Activity* a =
                    Activity::build((*it)->getAttributeValue("activity"));

                if (a->end()) {
                    mDoneActivities.push_back(a);
                    mPhase = SEND_DONE;
                } else {
                    if (mLocation == a->location().name()) {
                        mWaitingActivities.push_back(a);
                        mPhase = SEND_DEMAND;
                    } else {
                        mSchedulingActivities.push_back(a);
                        mPhase = SEND_SCHEDULE;
                    }
                }
            }
            ++it;
        }
    }

private:
    enum Phase { WAIT, SEND_DEMAND, SEND_DONE, SEND_PROCESS, SEND_SCHEDULE };

    std::string mLocation;

    Phase mPhase;
    ActivityFIFO mWaitingActivities;
    Activity* mRunningActivity;
    Activities mDoneActivities;
    Activities mSchedulingActivities;
};

} // namespace rcpsp

DECLARE_NAMED_DYNAMICS(StepScheduler, rcpsp::StepScheduler);
