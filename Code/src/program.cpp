/// @file

#include "program.hpp"

#define X_MIN           -1.0f
#define Y_MIN           -1.0f
#define X_MAX           1.0f
#define Y_MAX           1.0f
#define SIZE_X          81
#define SIZE_Y          81
#define NUM_POINTS      (SIZE_X*SIZE_Y)
#define DX              (float)((X_MAX-X_MIN)/SIZE_X)
#define DY              (float)((Y_MAX-Y_MIN)/SIZE_Y)
#define DT              0.005
#define KERNEL_FILE     "/Code/kernel/thekernel.cl"                             // Example of kernel. See note in setup().

kernel* k                 = new kernel();
kernel* k2                = new kernel();

float4* position_old      = new float4(NUM_POINTS);                             // Old position.
float4* velocity_old      = new float4(NUM_POINTS);                             // Old velocity.
float4* acceleration_old  = new float4(NUM_POINTS);                             // Old acceleration.

point4* position          = new point4(NUM_POINTS);                             // Position.
color4* color             = new color4(NUM_POINTS);                             // Particle color.
float4* velocity          = new float4(NUM_POINTS);                             // Velocity.
float4* acceleration      = new float4(NUM_POINTS);                             // Acceleration.

float4* gravity           = new float4(NUM_POINTS);                             // Gravity.
float4* stiffness         = new float4(NUM_POINTS);                             // Stiffness.
float4* resting           = new float4(NUM_POINTS);                             // Resting.
float4* friction          = new float4(NUM_POINTS);                             // Friction.
float4* mass              = new float4(NUM_POINTS);                             // Mass.

int1* index_PC            = new int1(NUM_POINTS);                               // Centre particle.
int1* index_PR            = new int1(NUM_POINTS);                               // Right particle.
int1* index_PU            = new int1(NUM_POINTS);                               // Up particle.
int1* index_PL            = new int1(NUM_POINTS);                               // Left particle.
int1* index_PD            = new int1(NUM_POINTS);                               // Down particle.

float4* freedom           = new float4(NUM_POINTS);                             // Freedom/constrain flag.

text4*  text              = new text4("neutrino!", 1.0f, 1.0f, 1.0f, 1.0f);

float tick;

void setup()
{
  int i;
  int j;
  float x;
  float y;

  /// NOTE: here the kernel file from the Neutrino example is loaded.
  /// The NEUTRINO_PATH environmental variable is used and concatenated with the
  /// file name of the example coming with Neutrino, relatively to the path
  /// where it is installed (pointed by NEUTRINO_PATH).
  /// In case another kernel has to be used, simply load it by means of:
  /// load_kernel("the_full_path_name_of_your_kernel");
  char full_name[32768];
  printf("path = %s\n", NEUTRINO_PATH);
  snprintf(full_name, sizeof full_name, "%s%s", NEUTRINO_PATH, KERNEL_FILE);

  k->source_file = full_name;                                                     // Loading OpenCL kernel source...
  k->size = NUM_POINTS;
  k->dimension = 1;

  load_kernel(k);
  init_opencl_kernel(k);                                                   // Initializing OpenCL kernel...


  k2->source_file = full_name;                                                     // Loading OpenCL kernel source...
  k2->size = NUM_POINTS;
  k2->dimension = 1;

  load_kernel(k2);
  init_opencl_kernel(k2);                                                   // Initializing OpenCL kernel...

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////// Preparing arrays... /////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  y = Y_MIN;

  for (j = 0; j < SIZE_Y; j++)
  {
    x = X_MIN;

    for (i = 0; i < SIZE_X; i++)
    {
      position_old->x[i + SIZE_X*j] = x;                                        // Setting "x" initial position_old...
      position_old->y[i + SIZE_X*j] = y;                                        // Setting "y" initial position_old...
      position_old->z[i + SIZE_X*j] = 0.0f;                                     // Setting "z" initial position_old...
      position_old->w[i + SIZE_X*j] = 1.0f;                                     // Setting "w" initial position_old...

      // Setting "x" initial position (Taylor's expansion)...
      position->x[i + SIZE_X*j] = position_old->x[i + SIZE_X*j] +               // position_old +
                                  velocity_old->x[i + SIZE_X*j]*DT +            // velocity_old*dt +
                                  0.5f*acceleration_old->x[i + SIZE_X*j]*DT*DT; // 1/2*acceleration_old*dt^2

      // Setting "y" initial position (Taylor's expansion)...
      position->y[i + SIZE_X*j] = position_old->y[i + SIZE_X*j] +               // position_old +
                                  velocity_old->y[i + SIZE_X*j]*DT +            // velocity_old*dt +
                                  0.5f*acceleration_old->y[i + SIZE_X*j]*DT*DT; // 1/2*acceleration_old*dt^2

      // Setting "z" initial position (Taylor's expansion)...
      position->z[i + SIZE_X*j] = position_old->z[i + SIZE_X*j] +               // position_old +
                                  velocity_old->z[i + SIZE_X*j]*DT +            // velocity_old*dt +
                                  0.5f*acceleration_old->z[i + SIZE_X*j]*DT*DT; // 1/2*acceleration_old*dt^2

      // Setting "w" initial position...
      position->w[i + SIZE_X*j] = 1.0f;

      gravity->x[i + SIZE_X*j] = 0.0f;                                          // Setting "x" gravity...
      gravity->y[i + SIZE_X*j] = 0.0f;                                          // Setting "y" gravity...
      gravity->z[i + SIZE_X*j] = 1.0f;                                         // Setting "z" gravity...
      gravity->w[i + SIZE_X*j] = 1.0f;                                          // Setting "w" gravity...

      stiffness->x[i + SIZE_X*j] = 10000.0f;                                       // Setting "x" stiffness...
      stiffness->y[i + SIZE_X*j] = 10000.0f;                                       // Setting "y" stiffness...
      stiffness->z[i + SIZE_X*j] = 10000.0f;                                       // Setting "z" stiffness...
      stiffness->w[i + SIZE_X*j] = 1.0f;                                        // Setting "w" stiffness...

      resting->x[i + SIZE_X*j] = DX;                                            // Setting "x" resting position...
      resting->y[i + SIZE_X*j] = DX;                                            // Setting "y" resting position...
      resting->z[i + SIZE_X*j] = DX;                                            // Setting "z" resting position...
      resting->w[i + SIZE_X*j] = 1.0f;                                          // Setting "w" resting position...

      friction->x[i + SIZE_X*j] = 0.1f;                                        // Setting "x" friction...
      friction->y[i + SIZE_X*j] = 0.1f;                                        // Setting "y" friction...
      friction->z[i + SIZE_X*j] = 0.1f;                                        // Setting "z" friction...
      friction->w[i + SIZE_X*j] = 1.0f;                                         // Setting "w" friction...

      mass->x[i + SIZE_X*j] = 0.25f;                                             // Setting "x" mass...
      mass->y[i + SIZE_X*j] = 0.25f;                                             // Setting "y" mass...
      mass->z[i + SIZE_X*j] = 0.25f;                                             // Setting "z" mass...
      mass->w[i + SIZE_X*j] = 0.25f;                                             // Setting "w" mass...

      color->r[i + SIZE_X*j] = 1.0f;                                            // Setting "x" initial color...
      color->g[i + SIZE_X*j] = 0.0f;                                            // Setting "y" initial color...
      color->b[i + SIZE_X*j] = 0.0f;                                            // Setting "z" initial color...
      color->a[i + SIZE_X*j] = 1.0f;                                            // Setting "w" initial color...

      index_PC->x[i + SIZE_X*j] =  i       + SIZE_X*j;

      freedom->x[i + SIZE_X*j] = 1.0f;
      freedom->y[i + SIZE_X*j] = 1.0f;
      freedom->z[i + SIZE_X*j] = 1.0f;
      freedom->w[i + SIZE_X*j] = 1.0f;

      if ((i != 0) && (i != (SIZE_X - 1)) && (j != 0) && (j != (SIZE_Y - 1)))   // When on bulk:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      else                                                                      // When on all borders:
      {
        gravity->x[i + SIZE_X*j] = 0.0f;                                        // Setting "x" gravity...
        gravity->y[i + SIZE_X*j] = 0.0f;                                        // Setting "y" gravity...
        gravity->z[i + SIZE_X*j] = 0.0f;                                        // Setting "z" gravity...
        gravity->w[i + SIZE_X*j] = 1.0f;                                        // Setting "w" gravity...

        freedom->x[i + SIZE_X*j] = 0.0f;
        freedom->y[i + SIZE_X*j] = 0.0f;
        freedom->z[i + SIZE_X*j] = 0.0f;
        freedom->w[i + SIZE_X*j] = 1.0f;
      }

      if ((i == 0) && (j != 0) && (j != (SIZE_Y - 1)))                          // When on left border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((i == (SIZE_X - 1)) && (j != 0) && (j != (SIZE_Y - 1)))               // When on right border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((j == 0) && (i != 0) && (i != (SIZE_X - 1)))                          // When on low border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((j == (SIZE_Y - 1)) && (i != 0) && (i != (SIZE_X - 1)))               // When on high border (excluding extremes):
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      if ((i == 0) && (j == 0))                                                 // When on low left corner:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((i == (SIZE_X - 1)) && (j == 0))                                      // When on low right corner:
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] =  i       + SIZE_X*(j + 1);
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
      }

      if ((i == 0) && (j == (SIZE_Y - 1)))                                      // When on high left corner:
      {
        index_PR->x[i + SIZE_X*j] = (i + 1)  + SIZE_X*j;
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PD->x[i + SIZE_X*j] = i       + SIZE_X*(j - 1);
      }

      if ((i == (SIZE_X - 1)) && (j == (SIZE_Y - 1)))                           // When on high right corner:
      {
        index_PR->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PU->x[i + SIZE_X*j] = index_PC->x[i + SIZE_X*j];
        index_PL->x[i + SIZE_X*j] = (i - 1)  + SIZE_X*j;
        index_PD->x[i + SIZE_X*j] =  i       + SIZE_X*(j - 1);
      }

      x += DX;
    }
    y += DY;
  }

  tick = 0.0f;                                                                  // Setting initial time tick...

  typeset(text);                                                                // Typesetting "text"...

  position->init();                                                             // Initializing kernel variable...
  color->init();                                                                // Initializing kernel variable...
  position_old->init();                                                         // Initializing kernel variable...
  velocity->init();                                                             // Initializing kernel variable...
  acceleration->init();                                                         // Initializing kernel variable...
  gravity->init();                                                              // Initializing kernel variable...
  stiffness->init();                                                            // Initializing kernel variable...
  resting->init();                                                              // Initializing kernel variable...
  friction->init();                                                             // Initializing kernel variable...
  mass->init();                                                                 // Initializing kernel variable...
  index_PC->init();                                                             // Initializing kernel variable...
  index_PR->init();                                                             // Initializing kernel variable...
  index_PU->init();                                                             // Initializing kernel variable...
  index_PL->init();                                                             // Initializing kernel variable...
  index_PD->init();                                                             // Initializing kernel variable...
  freedom->init();                                                              // Initializing kernel variable...

  position->set(k, 0);                                                          // Setting kernel argument #0...
  color->set(k, 1);                                                             // Setting kernel argument #1...
  position_old->set(k, 2);                                                      // Setting kernel argument #2...
  velocity->set(k, 3);                                                          // Setting kernel argument #3...
  acceleration->set(k, 4);                                                      // Setting kernel argument #4...
  gravity->set(k, 5);                                                           // Setting kernel argument #5...
  stiffness->set(k, 6);                                                         // Setting kernel argument #6...
  resting->set(k, 7);                                                           // Setting kernel argument #7...
  friction->set(k, 8);                                                          // Setting kernel argument #8...
  mass->set(k, 9);                                                              // Setting kernel argument #9...
  index_PC->set(k, 10);                                                         // Setting kernel argument #10...
  index_PR->set(k, 11);                                                         // Setting kernel argument #11...
  index_PU->set(k, 12);                                                         // Setting kernel argument #12...
  index_PL->set(k, 13);                                                         // Setting kernel argument #13...
  index_PD->set(k, 14);                                                         // Setting kernel argument #14...
  freedom->set(k, 15);                                                          // Setting kernel argument #15...
}

void loop()
{
  position->push(k, 0);                                                         // Pushing kernel argument #0...
  color->push(k, 1);                                                            // Pushing kernel argument #1...
  position_old->push(k, 2);                                                     // Pushing kernel argument #2...
  velocity->push(k, 3);                                                         // Pushing kernel argument #3...
  acceleration->push(k, 4);                                                     // Pushing kernel argument #4...
  gravity->push(k, 5);                                                          // Pushing kernel argument #5...
  stiffness->push(k, 6);                                                        // Pushing kernel argument #6...
  resting->push(k, 7);                                                          // Pushing kernel argument #7...
  friction->push(k, 8);                                                         // Pushing kernel argument #8...
  mass->push(k, 9);                                                             // Pushing kernel argument #9...
  index_PC->push(k, 10);                                                        // Pushing kernel argument #10...
  index_PR->push(k, 11);                                                        // Pushing kernel argument #11...
  index_PU->push(k, 12);                                                        // Pushing kernel argument #12...
  index_PL->push(k, 13);                                                        // Pushing kernel argument #13...
  index_PD->push(k, 14);                                                        // Pushing kernel argument #14...
  freedom->push(k, 15);                                                         // Pushing kernel argument #15...

  execute_kernel(k);

  position->pop(k, 0);                                                          // Popping kernel argument #0...
  color->pop(k, 1);                                                             // Popping kernel argument #1...
  position_old->pop(k, 2);                                                      // Popping kernel argument #2...
  velocity->pop(k, 3);                                                          // Popping kernel argument #3...
  acceleration->pop(k, 4);                                                      // Popping kernel argument #4...
  gravity->pop(k, 5);                                                           // Popping kernel argument #5...
  stiffness->pop(k, 6);                                                         // Popping kernel argument #6...
  resting->pop(k, 7);                                                           // Popping kernel argument #7...
  friction->pop(k, 8);                                                          // Popping kernel argument #8...
  mass->pop(k, 9);                                                              // Popping kernel argument #9...
  index_PC->pop(k, 10);                                                         // Popping kernel argument #10...
  index_PR->pop(k, 11);                                                         // Popping kernel argument #11...
  index_PU->pop(k, 12);                                                         // Popping kernel argument #12...
  index_PL->pop(k, 13);                                                         // Popping kernel argument #13...
  index_PD->pop(k, 14);                                                         // Popping kernel argument #14...
  freedom->pop(k, 15);                                                          // Popping kernel argument #15...

  plot(position, color, STYLE_POINT);                                           // Plotting points...
  print(text);                                                                  // Printing text...
}

void terminate()
{
  delete position;
  delete color;
  delete velocity;
  delete acceleration;
  delete position_old;
  delete velocity_old;
  delete acceleration_old;
  delete gravity;
  delete stiffness;
  delete resting;
  delete friction;
  delete mass;
  delete index_PC;
  delete index_PD;
  delete index_PL;
  delete index_PR;
  delete index_PU;

  delete text;
  release_kernel(k);                                                            // Releasing OpenCL kernel...
  release_program(k);                                                           // Releasing OpenCL program...

  release_kernel(k2);                                                           // Releasing OpenCL kernel...
  release_program(k2);                                                          // Releasing OpenCL program...

  printf("All done!\n");
}
