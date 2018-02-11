#include <cassert>
#include <cmath>
#include <climits>
#include <iostream>
#include <string>

#include "location.h"
#include "pendulum.h"

using namespace pendulumNames;
using namespace std;

const double kPi = 4*atan(1);
const Frequency kInvalidFrequency{-1,0};

bool operator==(const Position& l, const Position& r) {
  return l.x == r.x && l.y == r.y;
}

bool operator==(const Frequency& l, const Frequency& r) {
  return l.value == r.value && l.phase == r.phase;
}

void Frequency::ModulatePhase() {
  while (phase < 0) phase += Period();
  while (phase > Period()) phase -= Period();
}

double Frequency::Period() const { return 1/value; }

//Converts ratio to absolute value
void Frequency::SetStartPhase(double startPhase) {
  phase = startPhase*Period();
}

string Frequency::ToString() const {
  return "{freq:" + std::to_string(value) + ", phase:" + std::to_string(phase) + "}";
}

string Color::ToString() const {
  return "{R:" + std::to_string(R) + ", G:" + std::to_string(G) +
      ", B:" + std::to_string(B) + ", A:" + std::to_string(A) + "}";
}

string Position::ToString() const {
  return "{x:" + std::to_string(x) + ", y:" + std::to_string(y) + "}";
}

Position& Position::operator+=(const Position& rhs) {
  x += rhs.x;
  y += rhs.y;
  return *this;
}

Position& Position::operator-=(const Position& rhs) {
  x -= rhs.x;
  y -= rhs.y;
  return *this;
}

double pendulumNames::Norm(const Position& pos) { return sqrt(pos.x*pos.x + pos.y*pos.y); }

Position pendulumNames::operator+(Position lhs, const Position& rhs) {
  return lhs += rhs;
}

Position pendulumNames::operator-(Position lhs, const Position& rhs) {
  return lhs -= rhs;
}

Position pendulumNames::TranslateCenter(PendulumBase& pendulum, 
    double newx, double newy) {
  Position oldCenter = pendulum.center;
  pendulum.center = Position{newx, newy};
  return pendulum.center - oldCenter;
}

string CompoundPendulum::ToString() const {
  string str = "{" + name + ": ";
  for (const auto& p : pendulumList_) str += p->name + ", ";
  str.pop_back(); str.pop_back();
  str += ": center: " + center.ToString() + ", ";
  str += "color: " + color.ToString();
  str += "position: " + position.ToString();
  str += "}";
  return str;
}

void CompoundPendulum::UpdatePosition() {
  Position pos{0,0};
  for (const auto& p : pendulumList_) pos += (p->position - p->center);
  position = center + pos;
}

double SimplePendulum::WaveLength() const {
  return 2*kPi*amplitude;
}

SimplePendulum::SimplePendulum() : amplitude(1), direction{1,0}, 
      frequency(kInvalidFrequency), type(kInvalid) {}

double SimplePendulum::GetCycles() const { 
  switch(type) {
    case kOscillation : return .125; break;
    case kRotation : return .75; break;
    default : return 0;
  }
}

double SimplePendulum::GetPeriod() const { return frequency.Period(); }

string SimplePendulum::ToString() const {
  string str = "{name: " + name + ", ";
  str +=  "type: ";
  switch(type) {
    case kOscillation : str += "kOscillation, "; break;
    case kRotation : str += "kRotation, "; break;
    default : str += "kInvalid, ";
  }
  str += "amplitude: " + to_string(amplitude) + ", ";
  str += "center: " + center.ToString() + ", ";
  str += "color: " + color.ToString();
  str += "direction: " + direction.ToString() + ", ";
  str += "frequency: " + frequency.ToString() + ", ";
  str += "position: " + position.ToString() + ", ";
  str += "}";
  return str;
}

bool SimplePendulum::IsValid() const {
  bool valid = true;
  if (type == kInvalid) {
    cout << "Invalid Type" << endl;
    valid = false;
  } 
  if (frequency.value <= 0) {
    cout << "Invalid Frequency" << endl;
    valid = false;
  }
  if (amplitude <= 0) {
    cout << "Invalid amplitude" << endl;
    valid = false;
  }
  if (type == kOscillation && Norm(direction) == 0) {
    cout << "Invalid Direction" << endl;
    valid = false;
  }
  return valid;
}

//double PendulumBase::timeDelta = .20;

void SimplePendulum::UpdatePosition() {
  assert(IsValid());
  frequency.phase += timeDelta;
  frequency.ModulatePhase();
  double theta = frequency.phase*frequency.value;
  switch(type) {
    case kRotation :
      position.x = center.x + amplitude*cos(2*kPi*theta);
      position.y = center.y + amplitude*sin(2*kPi*theta); break;
    case kOscillation : {
      double norm = Norm(direction);
      double dx = direction.x/norm;
      double dy = direction.y/norm;
      position.x = center.x + dx*amplitude*sin(2*kPi*theta);
      position.y = center.y + dy*amplitude*sin(2*kPi*theta);
    } break;
    case kInvalid :
    default :
      assert(false && "SimplePendulum::UpdatePosition()");
  }
}

void SimplePendulum::SetPreferredBufferSize() {
  switch(type) {
  case SimplePendulum::kRotation : // draws 3/4 complete cycle
    preferredBufferSize = (size_t)round(.75*GetPeriod()/timeDelta); break;
  case SimplePendulum::kOscillation : // draws 1/8 complete cycle
     preferredBufferSize = (size_t)round(.125*GetPeriod()/timeDelta); break;
  default: assert(false);
  }
  cout << "Simple Buffer size: " << preferredBufferSize << endl;
}

void CompoundPendulum::SetPreferredBufferSize() {
  size_t maxBufSize = 0;
  for (const auto& p : pendulumList_) {
    maxBufSize = max(maxBufSize, p->preferredBufferSize);
  }
  preferredBufferSize = ((GetCycles() == 0) ? maxBufSize : GetCycles()*maxBufSize);
  cout << "Compound Buffer size: " << preferredBufferSize << endl;
}

double CompoundPendulum::GetCycles() const { 
  return cycles_;
}

bool CompoundPendulum::IsValid() const { return true; }

/*
int main() {
  double timeDelta = SimplePendulum::timeDelta;
  SimplePendulum pendulum;

  double period = .16;

  pendulum.frequency = {1/period,0};
  cout << "PENDULUM:\n" << pendulum.ToString() << endl;
  cout << "Is valid?" << endl;
  pendulum.IsValid();

  pendulum.color = {.5,.5,.5,.5};
  cout << "PENDULUM:\n" << pendulum.ToString() << endl;
  cout << "Is valid?" << endl;
  pendulum.IsValid();

  pendulum.position = {0,0};
  cout << "PENDULUM:\n" << pendulum.ToString() << endl;
  cout << "Is valid?" << endl;
  pendulum.IsValid();

  for (double phase = timeDelta; phase <= period; phase += timeDelta) {
    pendulum.UpdatePosition();
    cout << pendulum.ToString() << '\n' << endl;
  }
}

*/
