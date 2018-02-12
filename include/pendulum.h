//pendulum.h
#pragma once

#include <cmath>
#include <list>
#include <memory>
#include <string>

namespace pendulumNames {

using std::list;
using std::string;

struct Position {
  double x;
  double y;

  string    ToString() const;
  Position& operator+=(const Position& rhs);
  Position& operator-=(const Position& rhs);
};

Position operator+(Position lhs, const Position& rhs);
Position operator-(Position lhs, const Position& rhs);

double Norm(const Position& pos);

//note: a Position is a Direction...
typedef Position Direction;

/*
 * The frequency class uses the field "phase" to represent the
 * "current" frequency when performing calculations.  However,
 * in the input files, the values for phases indicate that "start
 * phase is $$ percentage of phase completed"
 *
 * units are in Hz
 */
class Frequency {
 public:
  double value;
  double phase;

  void   ModulatePhase();
  double Period() const;
  void   SetStartPhase(double startPhase);
  string ToString() const;
};

struct Color {
  double R,G,B,A;
  string ToString() const;
};

/*
 * A pendulum is the fundamental object being drawn by the
 * program.  The UpdatePosition() function is called by timeout
 * every time a "time-delta" has elapsed, then the new position
 * is added to a buffer somewhere, and an image of the pendulum's
 * path in space is drawn.
 */
class PendulumBase {
 public:
  Color    color;
  Position center;
  string   name;
  Position position;
  size_t   preferredBufferSize;

  static double timeDelta;

  virtual double GetCycles() const = 0; //should be "calculate cycles"
  virtual void   SetPreferredBufferSize() = 0;
  virtual string ToString()  const = 0;
  virtual void   UpdatePosition() = 0;
  /*
   * The following functions are here because of design errors.  Originally,
   * what is now "SimplePendulum" and "CompoundPendulum" were completely
   * different classes, but it seemed useful to combine them in order to
   * simplifiy their handling elsewhere.  But, there are some functions that
   * need to be available for only the SimplePendulum, so they are basically
   * dummies otherwise.
   */
  virtual bool IsValid() const = 0;
};

using PendulumPtr = std::unique_ptr<PendulumBase>;

/*
 * Simple pendulums: either oscillate or rotate.  Have frequency
 * and amplitude, but only oscillations have direction.
 */
class SimplePendulum : public PendulumBase {
 public:
  enum Type {kOscillation, kRotation, kInvalid};

  Type      type;
  double    amplitude;
  Direction direction;
  Frequency frequency;

  SimplePendulum();

  double GetCycles()  const override; //should be "calculate cycles"
  double GetPeriod()  const;
  bool   IsValid()    const override;
  void   SetPreferredBufferSize();
  string ToString()   const override;
  void   UpdatePosition()   override;
  double WaveLength() const;
};

class HarmonogramParser;

/*
 * Position given by sum of other pendulums.
 */
class CompoundPendulum : public PendulumBase {
 public:
  void    AddPendulum(PendulumBase* p) { pendulumList_.push_back(p); }
  double& cycles()                     { return cycles_; }
  double  GetCycles() const override; //should be "calculate cycles"
  bool    IsValid()   const override;
  void    SetPreferredBufferSize();
  string  ToString()  const override;
  void    UpdatePosition()  override;

  friend list<PendulumPtr> ReadCurrentInput(HarmonogramParser& parser);

 private:
  double cycles_;
  list<PendulumBase*> pendulumList_;
};

Position TranslateCenter(PendulumBase& pendulum, double newx, double newy);

}; //namespace pendulumNames
