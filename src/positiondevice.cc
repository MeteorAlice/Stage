///////////////////////////////////////////////////////////////////////////
//
// File: pioneermobiledevice.cc
// Author: Richard Vaughan, Andrew Howard
// Date: 5 Dec 2000
// Desc: Simulates the Pioneer robot base
//
// CVS info:
//  $Source: /home/tcollett/stagecvs/playerstage-cvs/code/stage/src/positiondevice.cc,v $
//  $Author: ahoward $
//  $Revision: 1.6 $
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#define ENABLE_RTK_TRACE 1

#include <math.h>

#include "world.hh"
#include "positiondevice.hh"

const double TWOPI = 6.283185307;


///////////////////////////////////////////////////////////////////////////
// Constructor
//
CPositionDevice::CPositionDevice(CWorld *world, CEntity *parent )
  : CEntity( world, parent )
{    
  // set the Player IO sizes correctly for this type of Entity
  m_data_len = sizeof( player_position_data_t );
  m_command_len = sizeof( player_position_cmd_t );
  m_config_len = 0;// not configurable
  
  m_player_type = PLAYER_POSITION_CODE; // from player's messages.h
  
  m_stage_type = RectRobotType;

  //m_render = layer_laser | layer_obstacle;
  
  uint64_t foo = 1;
  
  foo ++;

  m_com_vr = m_com_vth = 0;
  m_map_px = m_map_py = m_map_pth = 0;
  
  // Set the robot dimensions
  // Due to the unusual shape of the pioneer,
  // it is modelled as a rectangle offset from the origin
  //
  m_size_x = 0.440;
  m_size_y = 0.380;
  m_offset_x = -0.04;
  
  m_odo_px = m_odo_py = m_odo_pth = 0;
  
  stall = 0;
  
  m_interval = 0.01; // update this device VERY frequently
  
  // assume robot is 20kg
  m_mass = 20.0;
  
  // Set the default shape
  m_shape = rectangle;
  
#ifdef INCLUDE_RTK
    m_mouse_radius = 0.400;
    m_draggable = true;
#endif
}

///////////////////////////////////////////////////////////////////////////
// Load the object from an argument list
//
bool CPositionDevice::Load(int argc, char **argv)
{
    if (!CEntity::Load(argc, argv))
        return false;

    for (int i = 0; i < argc;)
    {
        // Extract shape
        //
        if (strcmp(argv[i], "shape") == 0 && i + 1 < argc)
        {
          if(!strcmp(argv[i+1],"rectangle"))
            SetShape(rectangle);
          else if(!strcmp(argv[i+1],"circle"))
            SetShape(circle);
          else
            PRINT_MSG1("pioneermobilebase: unknown shape \"%s\"; "
                       "defaulting to rectangle", argv[i+1]);
          i += 2;
        }
        else
            i++;
    }
    return true;
}

// Save the object
//
bool CPositionDevice::Save(int &argc, char **argv)
{
    if (!CEntity::Save(argc, argv))
        return false;
    
    argv[argc++] = strdup("shape");
    
    switch( m_shape )
      {
      case circle:
	argv[argc++] = strdup("circle");
	break;
      case rectangle:
	argv[argc++] = strdup("rectangle");
	break;
      default:
	printf( "Warning: wierd position device shape (%d)\n", m_shape );
	argv[argc++] = strdup("rectangle");
      }
	
    return true;
}


///////////////////////////////////////////////////////////////////////////
// Update the position of the robot base
//
void CPositionDevice::Update( double sim_time )
{
    // If the device is not subscribed,
    // reset to default settings.
    //
  if(!Subscribed())
  {
    m_odo_px = m_odo_py = m_odo_pth = 0;
    return;
  }
  
    ASSERT(m_world != NULL);
  
    // if its time to recalculate state
    //
    if( sim_time - m_last_update <  m_interval )
      return;
    
    m_last_update = sim_time;

    // Get the latest command
    //
    if( GetCommand( &m_command, sizeof(m_command)) == sizeof(m_command))
    {
        ParseCommandBuffer();    // find out what to do    
    }
 
    Map(false); // erase myself
   
    Move();      // do things
    
    Map(true);   // draw myself 

    ComposeData();     // report the new state of things
    PutData( &m_data, sizeof(m_data)  );     // generic device call

}


int CPositionDevice::Move()
{
    double step_time = m_world->GetTime() - m_last_time;
    m_last_time += step_time;

    // Get the current robot pose
    //
    double px, py, pth;
    GetPose(px, py, pth);

    // Compute a new pose
    // This is a zero-th order approximation
    //
    double qx = px + m_com_vr * step_time * cos(pth);
    double qy = py + m_com_vr * step_time * sin(pth);
    double qth = pth + m_com_vth * step_time;

    // Check for collisions
    // and accept the new pose if ok
    //
    if (InCollision(qx, qy, qth))
    {
        this->stall = 1;
    }
    else
    {
        SetPose(qx, qy, qth);
        this->stall = 0;
    }
        
    // Compute the new odometric pose
    // Uses a first-order integration approximation
    //
    double dr = m_com_vr * step_time;
    double dth = m_com_vth * step_time;
    m_odo_px += dr * cos(m_odo_pth + dth / 2);
    m_odo_py += dr * sin(m_odo_pth + dth / 2);
    m_odo_pth += dth;

    // Normalise the odometric angle
    //
    m_odo_pth = fmod(m_odo_pth + TWOPI, TWOPI);
    
    return true;
}


///////////////////////////////////////////////////////////////////////////
// Parse the command buffer to extract commands
//
void CPositionDevice::ParseCommandBuffer()
{
    double fv = (short) ntohs(m_command.speed);
    double fw = (short) ntohs(m_command.turnrate);

    // Store commanded speed
    // Linear is in m/s
    // Angular is in radians/sec
    //
    m_com_vr = fv / 1000;
    m_com_vth = DTOR(fw);
}


///////////////////////////////////////////////////////////////////////////
// Compose data to send back to client
//
void CPositionDevice::ComposeData()
{
    // Compute odometric pose
    // Convert mm and degrees (0 - 360)
    //
    double px = m_odo_px * 1000.0;
    double py = m_odo_py * 1000.0;
    double pth = RTOD(fmod(m_odo_pth + TWOPI, TWOPI));

    // Get actual global pose
    //
    double gx, gy, gth;
    GetGlobalPose(gx, gy, gth);
    
    // normalized compass heading
    //
    double compass = fmod( gth + M_PI/2.0 + TWOPI, TWOPI ); 
  
    // Construct the data packet
    // Basically just changes byte orders and some units
    //
    m_data.xpos = htonl((int) px);
    m_data.ypos = htonl((int) py);
    m_data.theta = htons((unsigned short) pth);

    m_data.speed = htons((unsigned short) (m_com_vr * 1000.0));
    m_data.turnrate = htons((short) RTOD(m_com_vth));  
    m_data.compass = htons((unsigned short)(RTOD(compass)));
    m_data.stalls = this->stall;
}


///////////////////////////////////////////////////////////////////////////
// Check to see if the given pose will yield a collision
//
bool CPositionDevice::InCollision(double px, double py, double pth)
{
  if(GetShape() == rectangle)
  {
    double qx = px + m_offset_x * cos(pth);
    double qy = py + m_offset_x * sin(pth);
    double sx = m_size_x;
    double sy = m_size_y;

    if (m_world->GetRectangle(qx, qy, pth, sx, sy, layer_obstacle) > 0)
        return true;
    
  }
  else if(GetShape() == circle)
  {
    if (m_world->GetRectangle(px, py, pth, m_size_x, m_size_x,  // CIRCLE! 
                              layer_obstacle) > 0)
        return true;
  }
  else
    PRINT_MSG("CPositionDevice::InCollision(): unknown shape!");
  return false;
}

void CPositionDevice::SetShape(pioneer_shape_t shape)
{
  m_shape = shape;
   
  switch( m_shape )
    {
    case circle:  
      m_stage_type = RoundRobotType;
      m_size_y = m_size_x;
      break;
    case rectangle:
      m_stage_type = RectRobotType;
    }
}


///////////////////////////////////////////////////////////////////////////
// Render the object in the world rep
//
void CPositionDevice::Map(bool render)
{
    if (!render)
    {
        if(GetShape() == rectangle)
        {
          // Remove ourself from the obstacle map
          //
          double px = m_map_px;
          double py = m_map_py;
          double pa = m_map_pth;

          double qx = px + m_offset_x * cos(pa);
          double qy = py + m_offset_x * sin(pa);
          double sx = m_size_x;
          double sy = m_size_y;
          m_world->SetRectangle(qx, qy, pa, sx, sy, layer_obstacle, 0);
          m_world->SetRectangle(qx, qy, pa, sx, sy, layer_puck, 0);
        }
        else if(GetShape() == circle)
        {
          m_world->SetCircle(m_map_px,m_map_py,m_size_x/2.0,layer_obstacle,0);
          m_world->SetCircle(m_map_px,m_map_py,m_size_x/2.0,layer_puck,0);
        }
        else
          PRINT_MSG("CPositionDevice::Map(): unknown shape!");
    }
    else
    {
      // Add ourself to the obstacle map
      //
      double px, py, pa;
      GetGlobalPose(px, py, pa);
      if(GetShape() == rectangle)
      {
        double qx = px + m_offset_x * cos(pa);
        double qy = py + m_offset_x * sin(pa);
        double sx = m_size_x;
        double sy = m_size_y;
        m_world->SetRectangle(qx, qy, pa, sx, sy, layer_obstacle, 1);
        m_world->SetRectangle(qx, qy, pa, sx, sy, layer_puck, 1);
      }
      else if(GetShape() == circle)
      {
          m_world->SetCircle(px,py,m_size_x/2.0,layer_obstacle,1);
          m_world->SetCircle(px,py,m_size_x/2.0,layer_puck,1);
      }
      else
        PRINT_MSG("CPositionDevice::Map(): unknown shape!");

      // Store the place we added ourself
      //
      m_map_px = px;
      m_map_py = py;
      m_map_pth = pa;
    }
}



#ifdef INCLUDE_RTK

///////////////////////////////////////////////////////////////////////////
// Process GUI update messages
//
void CPositionDevice::OnUiUpdate(RtkUiDrawData *data)
{
    CEntity::OnUiUpdate(data);
    
    data->begin_section("global", "");
    
    if (data->draw_layer("chassis", true))
        DrawChassis(data);
    
    data->end_section();
}


///////////////////////////////////////////////////////////////////////////
// Process GUI mouse messages
//
void CPositionDevice::OnUiMouse(RtkUiMouseData *data)
{
    CEntity::OnUiMouse(data);
}


///////////////////////////////////////////////////////////////////////////
// Draw the pioneer chassis
//
void CPositionDevice::DrawChassis(RtkUiDrawData *data)
{
    #define ROBOT_COLOR RTK_RGB(255, 0, 192)
    
    data->set_color(ROBOT_COLOR);

    // Get global pose
    //
    double px, py, pa;
    GetGlobalPose(px, py, pa);

    if(GetShape() == rectangle)
    {
      // Compute an offset
      //
      double qx = px + m_offset_x * cos(pa);
      double qy = py + m_offset_x * sin(pa);
      double sx = m_size_x;
      double sy = m_size_y;
      data->ex_rectangle(qx, qy, pa, sx, sy);

      // Draw the direction indicator
      //
      for (int i = 0; i < 3; i++)
      {
        double px = min(sx, sy) / 2 * cos(DTOR(i * 45 - 45));
        double py = min(sx, sy) / 2 * sin(DTOR(i * 45 - 45));
        double pth = 0;

        // This is ugly, but it works
        //
        if (i == 1)
          px = py = 0;

        LocalToGlobal(px, py, pth);

        if (i > 0)
          data->line(qx, qy, px, py);
        qx = px;
        qy = py;
      }
    }
    else if(GetShape() == circle)
    {
      data->ellipse(px - m_size_x/2.0, py - m_size_x/2.0, 
                    px + m_size_x/2.0, py + m_size_x/2.0);
      
      // Draw the direction indicator
      //
      for (int i = 0; i < 3; i++)
      {
        double qx = m_size_x/2.0 * cos(DTOR(i * 45 - 45));
        double qy = m_size_x/2.0 * sin(DTOR(i * 45 - 45));
        double qth = 0;

        // This is ugly, but it works
        //
        if (i == 1)
          qx = qy = 0;

        LocalToGlobal(qx, qy, qth);

        if (i > 0)
          data->line(px, py, qx, qy);
        px = qx;
        py = qy;
      }
    }
    else
      PRINT_MSG("CPositionDevice::DrawChassis(): unknown shape!");
}

#endif
