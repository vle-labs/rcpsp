/**
 * @file Activity.hpp
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

#ifndef __ACTIVITY_HPP
#define __ACTIVITY_HPP 1

#include <string>
#include <vector>

#include <vle/value/Value.hpp>

#include <Resources.hpp>
#include <Steps.hpp>
#include <TemporalConstraints.hpp>

namespace rcpsp {

class Activity
{
public:
    Activity(const std::string& name,
             const TemporalConstraints& temporalConstraints) :
        mName(name), mSteps(new Steps()),
        mTemporalConstraints(temporalConstraints),
        mAllocatedResources(0)
    { }

    Activity(const Activity& a) : mName(a.mName),
                                  mSteps(new Steps(*a.mSteps)),
                                  mTemporalConstraints(a.mTemporalConstraints),
                                  mAllocatedResources(a.mAllocatedResources)
    {
        if (not mSteps->empty() and a.mStepIterator != a.mSteps->end()) {
            mStepIterator = mSteps->find((*a.mStepIterator)->name());
        } else {
            mStepIterator = mSteps->begin();
        }
    }

    Activity(const vle::value::Value* value);

    virtual ~Activity()
    { delete mSteps; }

    void addStep(Step* step)
    { mSteps->push_back(step); }

    const Resources* allocatedResources() const
    { return mAllocatedResources; }

    void assign(Resources* r)
    { mAllocatedResources = r; }

    static Activity* build(const vle::value::Value& value)
    { return new Activity(&value); }

    const Step* current() const
    { return *mStepIterator; }

    bool done(const vle::devs::Time& time) const;

    bool end() const
    { return mStepIterator == mSteps->end(); }

    void finish(const vle::devs::Time& time);

    const Location& location() const
    { return (*mStepIterator)->location(); }

    const std::string& name() const
    { return mName; }

    const Activity& operator=(const Activity& a)
    {
        mName = a.mName;
        mSteps = new Steps(*a.mSteps);
        mTemporalConstraints = a.mTemporalConstraints;
        if (not mSteps->empty() and a.mStepIterator != a.mSteps->end()) {
            mStepIterator = mSteps->find((*a.mStepIterator)->name());
        } else {
            mStepIterator = mSteps->begin();
        }
        return *this;
    }

    void release()
    { mAllocatedResources = 0; }

    vle::devs::Time remainingTime(const vle::devs::Time& time) const;

    const ResourceConstraints& resourceConstraints() const;

    void start(const vle::devs::Time& time);

    bool starting(const vle::devs::Time& time) const;

    const TemporalConstraints& temporalConstraints() const
    { return mTemporalConstraints; }

    vle::value::Value* toValue() const;

    void wait(const vle::devs::Time& time);

private:
    friend std::ostream& operator<<(std::ostream& o, const Activity& a);

    std::string mName;
    Steps* mSteps;
    TemporalConstraints mTemporalConstraints;

    // state
    Steps::iterator mStepIterator;
    Resources* mAllocatedResources;
    bool mWaiting;
    bool mRunning;
    bool mDone;
};

} // namespace rcpsp

#endif
