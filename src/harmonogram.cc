//#include <gtkmm-3.0/gtkmm.h>

#include <glibmm-2.4/glibmm.h>

#include <gtkmm-3.0/gtkmm/adjustment.h>
#include <gtkmm-3.0/gtkmm/drawingarea.h>
#include <gtkmm-3.0/gtkmm/scrolledwindow.h>
#include <gtkmm-3.0/gtkmm/window.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <thread>
#include <utility>
#include <vector>

#include "location.h"
#include "pendulum.h"
#include "pendulum_parser.h"
#include "ringbuffer.h"
//#include "vimserver.h"

using namespace std;
using namespace pendulumNames;
//using namespace vimserverNames;

//Globals

double defaultDelta = .01;
double PendulumBase::timeDelta = defaultDelta;
double& timeDelta = PendulumBase::timeDelta;

enum ProgramState { kRunning, kIdle, kStopped };
ProgramState state = kStopped;
ProgramState prevState;

class PendulumDrawer;
PendulumDrawer* lastClickedPendulum;
PendulumDrawer* currentHighlightPendulum;

list<string> fileNameList;
//

class Harmonogram;
bool UpdateHighlightPendulum(Harmonogram* harmonogram);

/*
 * ----Color Effects
 */

void ColorFade(Color& color, const double& factor)
{
  //fade:
  color.A*=factor;
}

Color centerColor = {.2,.2,.4,0};

void UpdateCenterColor()
{
  static double time = 0;
  centerColor.A = .75 + .25*sin(2*M_PI*time);
  time += timeDelta;
}

/*
 * Rainbow coloring goes:
 *  (1,0,0) -> (1,1,0) -> (0,1,0) -> (0,1,1) -> (0,0,1) -> (1,0,1) -> (loop)
 *   Red        Yellow     Green      Cyan       Blue       Magenta
 */
enum RainbowDirection {
  GoingToYellow,
  GoingToGreen,
  GoingToCyan,
  GoingToBlue,
  GoingToMagenta,
  GoingToRed
};

/*
 * ColorRainbow
 */
void ColorRainbow(
    Color& color,
    RainbowDirection& rainbowDirection,
    const double& increment) 
{
  switch(rainbowDirection)
  {
    case GoingToYellow :
      color.G += increment;
      if (color.G >= 1)
      {
        rainbowDirection = GoingToGreen;
      }
      break;
    case GoingToGreen : 
      color.R -= increment;
      if (color.R <= 0)
      {
        rainbowDirection = GoingToCyan;
      }
      break;
    case GoingToCyan : 
      color.B += increment;
      if (color.B >= 1)
      {
        rainbowDirection = GoingToBlue;
      }
      break;
    case GoingToBlue : 
      color.G -= increment;
      if (color.G <= 0)
      {
        rainbowDirection = GoingToMagenta;
      }
      break;
    case GoingToMagenta : 
      color.R += increment;
      if (color.R >= 1)
      {
        rainbowDirection = GoingToRed;
      }
      break;
    case GoingToRed : 
      color.B -= increment;
      if (color.B <= 0)
      {
        rainbowDirection = GoingToYellow;
      }
      break;
  }
}

/*
 * --End Color Effects
 */

/*
 * Holds a PendulumBase pointer and does all of the necessary drawing
 * functions for it.  This includes keeping a RingBuffer for the positions as it
 * evoloves through time.
 *
 * void Draw(context);
 * void Update();
 * Position GetCenter(); //returns by value!
 * PendulumBase* GetPendulum();
 * void UpdateCenter(Position);
 *
 * void Resize(); //Resizes Ring buffer
 */
class PendulumDrawer {
 public:
  enum Style { kPlain, kFade, kRainbow};
  RainbowDirection rainbowDirection;

  PendulumDrawer(PendulumBase* pendulum) : 
      pendulum_(pendulum), style_(kPlain) 
  {
    Update();
    positionBuffer_.Fill(pendulum_->preferredBufferSize, pendulum_->position);
    fadeFactor_ = exp2(log2(.05)/(double)positionBuffer_.buffer.size());
    colorIncrement_ = pendulum_->GetCycles()/(double)positionBuffer_.buffer.size();
    cout << "color increment: " << colorIncrement_ << endl;
  }

  /*
   * FadeDraw
   */
  void FadeDraw(const Cairo::RefPtr<Cairo::Context>& c)
  {
    Color startColor = pendulum_->color;
    for (auto pos = positionBuffer_.end(); pos != positionBuffer_.begin(); )
    {
      c->move_to(pos->x, pos->y);
      --pos;
      c->set_source_rgba(startColor.R, startColor.G, startColor.B, startColor.A);
      ColorFade(startColor, fadeFactor_);
      c->line_to(pos->x, pos->y);
      c->stroke();
    }
  }
  /*
   * PlainDraw
   */
  void PlainDraw(const Cairo::RefPtr<Cairo::Context>& c)
  {
    Color startColor = pendulum_->color;
    c->move_to(positionBuffer_.front().x, positionBuffer_.front().y);
    c->set_source_rgba(startColor.R, startColor.G, startColor.B, startColor.A);
    for (auto pos : positionBuffer_) c->line_to(pos.x, pos.y);
    c->stroke();
  }
  /*
   * RainbowDraw
   */
  void RainbowDraw(const Cairo::RefPtr<Cairo::Context>& c)
  {
    Color startColor = pendulum_->color;
    RainbowDirection startDirection = rainbowDirection;
    for (auto pos = positionBuffer_.begin(); pos != positionBuffer_.end(); )
    {
      c->move_to(pos->x, pos->y);
      ++pos;
      c->set_source_rgba(startColor.R, startColor.G, startColor.B, startColor.A);
      ColorRainbow(startColor, startDirection, colorIncrement_);
      c->line_to(pos->x, pos->y);
      c->stroke();
    } 
    ColorRainbow(pendulum_->color, rainbowDirection, colorIncrement_);
  }
  /*
   * CenterDraw
   */
  void CenterDraw(const Cairo::RefPtr<Cairo::Context>& c)
  {
    //cout << __func__ << endl;
    c->set_source_rgba(centerColor.R,centerColor.G,centerColor.B,centerColor.A);
    c->arc(pendulum_->center.x, pendulum_->center.y, 15, 0, 2*M_PI);
    c->fill();
    UpdateCenterColor();
  }
  /*
   * Draw
   */
  void Draw(const Cairo::RefPtr<Cairo::Context>& c)
  {
    c->save();
    c->set_line_width(3);
    switch (state)
    {
      case kRunning :
        switch (style_)
        {
          case kPlain : PlainDraw(c); break;
          case kFade : FadeDraw(c); break;
          case kRainbow: RainbowDraw(c);
        } break;
      case kIdle : 
      case kStopped :
        PlainDraw(c); 
        CenterDraw(c); break;
    }
    c->restore();
  }
  /*
   * Update
   */
  void Update()
  {
    pendulum_->UpdatePosition();
    positionBuffer_.Push(pendulum_->position);
  }

  Position      GetCenter()   { return pendulum_->center; }
  PendulumBase* GetPendulum() { return pendulum_; }
  string        Name()        { return pendulum_->name; }
  /*
   * UpdateCenter
   */
  void          UpdateCenter(double x, double y)
  {
    positionBuffer_.Translate(TranslateCenter(*pendulum_, x, y));
  }
  /*
   * Resize
   */
  void          Resize(size_t size)
  {
    positionBuffer_.Fill(size, pendulum_->center);
  }
  /*
   * NextStyle
   */
  void          NextStyle()
  {
    switch (style_)
    {
      case kPlain : style_ = kFade; break;
      case kFade : style_ = kRainbow; 
                   pendulum_->color = {1,0,0,1};
                   rainbowDirection = GoingToYellow;
                   break;
      case kRainbow : style_ = kPlain; break;
    }
  }

  private:
    PendulumBase* pendulum_;
    RingBuffer<Position> positionBuffer_;
    double fadeFactor_;
    double colorIncrement_;
    Style style_;
};

/*
 * The Worker class for the program.  Holds a list of PendulumBases and reacts
 * to events to maintain them.  The events are clearly seen at the beginning of
 * the default constructor.
 *
 * string PrintData();
 * bool CheckForPendulumAt(x,y); 
 *      //returns true when a pendulum's center is close enough to x,y
 * void Initialize();
 * void ReRead(); //reparses the input files, resets all pendulums
 * void UpdateAll(); //calls Update on all pendulumDrawers
 *
 */
class Harmonogram : public Gtk::DrawingArea {
  public:
    Harmonogram()  /*: vimServer("Harmonogram")*/ {
      //signals

      //time evolution
      Glib::signal_timeout().connect(
          sigc::mem_fun(*this, &Harmonogram::on_timeout), round(1000*timeDelta));

      //select a pendulum 
      signal_button_press_event().connect(
          sigc::mem_fun(*this, &Harmonogram::on_button_press_event)); 

      signal_button_release_event().connect(
          sigc::mem_fun(*this, &Harmonogram::on_button_release_event)); 

      //drag a pendulum
      signal_motion_notify_event().connect(
          sigc::mem_fun(*this, &Harmonogram::on_motion_notify_event));

      add_events(Gdk::EventMask::KEY_PRESS_MASK |
          Gdk::EventMask::BUTTON_RELEASE_MASK |
          Gdk::EventMask::BUTTON_PRESS_MASK |
          Gdk::EventMask::POINTER_MOTION_MASK);

      //Set up structures from reading the input file
      ReRead();
      //vimServer.SetFileNameList(fileNameList);
      //vimServer.Activate();
    }

    void PrintData()
    {
      for (const auto& pendulumPtr : pendulumList_)
      {
        cout << pendulumPtr->ToString() << endl;
      }
    } 

    void ReRead()
    {
      pendulumList_.clear();
      pendulumDrawerList_.clear();
      pendulumList_ = ::std::move(harmonogramParser_.Parse(fileNameList));
      Initialize();
    }

    void UpdateHP()
    {
      static thread* old_thread = nullptr;
      thread* tp = new thread(UpdateHighlightPendulum, this);
      if (old_thread) old_thread->join();
      old_thread = tp;
    }

    friend bool UpdateHighlightPendulum(Harmonogram*);

  private:

    /*
     * bool CheckForPendulumAt(double x, double y)
     */
    bool CheckForPendulumAt(double x, double y)
    {
      static const double tolerance = 15;
      for (auto& p : pendulumDrawerList_)
      {
        if (Norm(p.GetPendulum()->center - Position{x,y}) < tolerance)
        {
          lastClickedPendulum = &p;
          return true;
        }
      }
      return false;
    }
    /*
     * Initialize
     */
    void Initialize()
    {
      state = kRunning;
      for (PendulumPtr& pendulumPtr : pendulumList_)
      {
        pendulumDrawerList_.emplace_back(pendulumPtr.get());
        cout << pendulumPtr->ToString() << endl;
      }
    }
    /*
     * VimGotoPendulum
     */
    void VimGotoPendulum()
    {
      cout << __func__ << endl;
      if (!lastClickedPendulum) return;
      cout << "last clicked pendulum: " << lastClickedPendulum->Name() << endl;
      //vimServer.SetCursor(
      //harmonogramParser_.locationMap[lastClickedPendulum->Name()].begin);
      //if (lastHighlighted != "")
      //{
        //vimServer.UnHighlightPattern(lastHighlighted);
        //}
      lastHighlighted = lastClickedPendulum->Name();
      //vimServer.HighlightPattern(lastHighlighted);
    }
    /*
     * on_button_press_event
     */
    bool on_button_press_event(GdkEventButton* button)
    {
      //double clicks can get program stuck...
      if (state == prevState)
      {state = kRunning; }
      if (button->type != GDK_BUTTON_PRESS) return false;
      if (CheckForPendulumAt(button->x, button->y))
      {
        cout << "button: " << button->button << endl;
        switch (button->button)
        {
          case 1 :
            lastClickedPendulum->NextStyle();
            prevState = state;
            //stop time!
            timeDelta = 0;
            state = kIdle;
            return true; break;
          case 2 : return false; break;
                   /*case 3 : if (!vimServer.CheckServer()) vimServer.Activate();
                     vimServer.setNormal = vimServer.SetNormalMode();
                     VimGotoPendulum();
                     UpdateHP();
                     vimServer.setNormal = false;
                     return true; break;*/
          default : return false;
        }
      } else {
        return false;
      } 
    }
    /*
     * on_button_release_event
     */
    bool on_button_release_event(GdkEventButton* button)
    {
      switch (state)
      {
        case kIdle: //start time!
          timeDelta = defaultDelta;
          state = prevState; break;
        default:
          return false;
      }
      return true;
    }
    /*
     * on_draw
     */
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& c)
    {
      for (PendulumDrawer& p : pendulumDrawerList_) p.Draw(c);
      //if (!vimServer.IsActive()) return true;
      if (currentHighlightPendulum)
        currentHighlightPendulum->CenterDraw(c);
      return true;
    }
    /*
     * on_motion_notify_event
     */
    bool on_motion_notify_event(GdkEventMotion* motion)
    {
      switch(state)
      {
        case kIdle :
          if (!lastClickedPendulum) return false;
          lastClickedPendulum->UpdateCenter(motion->x, motion->y); break;
        default :
          return false;
      }
      return true;
    }
    /*
     * on_timeout
     */
    bool on_timeout()
    {
      switch (state)
      {
        case kRunning :
          UpdateAll();
          break;
        case kIdle :
        case kStopped :
          break;
      }
      queue_draw();
      return true;
    }
    /*
     * UpdateAll
     */
    void UpdateAll()
    {
      for (auto& p : pendulumDrawerList_) p.Update();
    }

    HarmonogramParser harmonogramParser_;
    list<PendulumPtr> pendulumList_;
    list<PendulumDrawer> pendulumDrawerList_;

    string lastHighlighted;
    const double waitPeriod = 2; //seconds
    double curTime = 0;
    //VimServer vimServer;
};

/*
 * This is where the magic finally happens.  Registers events for stuff.
 * Captures the following keys:
 * <space> : switch from RUNNING mode to IDLE.
 *  Basically pauses motion while dragging pendulums around
 * <r> : ReRead the input files.
 */
class MyWindow : public Gtk::Window {
  public:
    MyWindow()
    {
      signal_key_press_event().connect(
          sigc::mem_fun(*this, &MyWindow::on_key_press_event));

      signal_button_press_event().connect(
          sigc::mem_fun(*this, &MyWindow::on_button_press_event));

      add_events(Gdk::EventMask::KEY_PRESS_MASK | 
          Gdk::EventMask::BUTTON_PRESS_MASK |
          Gdk::EventMask::BUTTON_RELEASE_MASK |
          Gdk::EventMask::POINTER_MOTION_MASK);

      add(harmonogram_);

      override_background_color(Gdk::RGBA("rgba(30,30,30,1)"));
      set_default_size(800,600);

      show_all_children();
      show();
    }

  private:
    Harmonogram harmonogram_;

    /*
     * bool on_button_press_event(GdkEventButton* button) {
     */
    bool on_button_press_event(GdkEventButton* button) {
      return false; 
    }
    /*
     * bool on_key_press_event(GdkEventKey* key)
     */
    bool on_key_press_event(GdkEventKey* key)
    {
      cout << "MyWindow: key pressed: " << key->keyval << endl;

      if (key->keyval == GDK_KEY_space)
      {
        cout << "space pressed, state: " << state << endl;
        ToggleStart();

        if (state == kStopped) harmonogram_.PrintData();
        return true;

      } else if (key->keyval == GDK_KEY_r) {

        harmonogram_.ReRead();
        return true;

      } else if (key->keyval == GDK_KEY_h) {

        harmonogram_.UpdateHP();
        return true;
      }
      return false;
    }
    /*
     * void ToggleStart()
     */
    void ToggleStart()
    {
      switch (state)
      {
        case kRunning : state = kStopped; break;
        case kStopped : state = kRunning; break;
        default: break;
      }
    }
};

bool UpdateHighlightPendulum(Harmonogram* harmonogram)
{
  Location curPos;
  PendulumDrawer* harmPtr = nullptr;
  //curTime += timeDelta;
  //if (curTime < waitPeriod) return false;
  //curTime -= waitPeriod;
  /* if the cursor is successfully loaded from the vimServer, then
   * find the PendulumId which corresponds to the Range which contains
   * that cursor Postion (if it exists), then find which PendulumDrawer
   * has a pendulum_ which has the same PendulumID, if it exists, then
   * set this as the currentHighlightPendulum, so that it's center can
   * be drawn.
   */
  //map<PendulumId,Range>::value_type* val = nullptr;
  /*if (harmonogram->vimServer.GetCursor(curPos))
    {
    for (auto& p : harmonogram->harmonogramParser_.locationMap) 
    if (p.second.InRange(curPos)) 
    for (auto& pDrawer : harmonogram->pendulumDrawerList_)
    {
    if (!val && pDrawer.Name() == p.first)
    {
    currentHighlightPendulum = &pDrawer;
    val = &p;
    cout << "curPtr: " << pDrawer.Name() << endl;
    } else if (val && pDrawer.Name() == p.first)
    {
    if (val->second.IsSubrange(p.second))
    {
    currentHighlightPendulum = &pDrawer;
    cout << "setting curHPend: " << pDrawer.Name() << endl;
    }
    return true;
    }
    }
    }*/
  if (harmPtr)
  {
    currentHighlightPendulum = harmPtr;
    cout << "current set: " << currentHighlightPendulum->Name() << endl;
    return true;
  }
  cout << "currentHighlightPendulum not set!" << endl;
  currentHighlightPendulum = nullptr;
  return false;
}

int main(int argc, char** argv)
{
  timeDelta = defaultDelta;

  if (argc > 1)
  {
    for (int i = 1; i < argc; ++i) 
      fileNameList.push_back(argv[i]);
  } else {
    cout << "please enter input files" << endl;
    return 0;
  }

  auto app = Gtk::Application::create("Harmonogram.Window");

  MyWindow window;
  app->run(window);
}
