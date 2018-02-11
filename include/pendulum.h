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

  string ToString() const;
  Position& operator+=(const Position& rhs);
  Position& operator-=(const Position& rhs);
};

Position operator+(Position lhs, const Position& rhs);
Position operator-(Position lhs, const Position& rhs);

double Norm(const Position& pos);

//note: a Position is a Direction...
typedef Position Direction;

//units are Hz
class Frequency {
 public:
  double value;
  double phase;

  void ModulatePhase();
  double Period() const;
  void SetStartPhase(double startPhase);
  string ToString() const;
};

struct Color {
  double R,G,B,A;
  string ToString() const;
};

class PendulumBase {
 public:
  Color color;
  Position center;
  string name;
  Position position;
  size_t preferredBufferSize;

  static double timeDelta;

  virtual double GetCycles() const = 0;
  virtual void UpdatePosition() = 0;
  virtual string ToString() const = 0;
  virtual void SetPreferredBufferSize() = 0;
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

class SimplePendulum : public PendulumBase {
 public:
  enum Type {kOscillation, kRotation, kInvalid};

  double amplitude;
  Direction direction;
  Frequency frequency;
  //Direction for oscillation
  Type type;

  SimplePendulum();
  double GetCycles() const override;
  double GetPeriod() const;
  bool IsValid() const override;
  void SetPreferredBufferSize();
  string ToString() const override;
  void UpdatePosition() override;
  double WaveLength() const;
};

 class HarmonogramParser;

class CompoundPendulum : public PendulumBase {
 public:
  void AddPendulum(PendulumBase* p) { pendulumList_.push_back(p); }
  double GetCycles() const override;
  bool IsValid() const override;
  void SetPreferredBufferSize();
  string ToString() const override;
  void UpdatePosition() override;

  friend list<PendulumPtr> ReadCurrentInput(HarmonogramParser& parser);

  double cycles_;
 private:
  list<PendulumBase*> pendulumList_;
};

//returns the amount shifted
Position TranslateCenter(PendulumBase& pendulum, double newx, double newy);

}; //namespace pendulumNames
