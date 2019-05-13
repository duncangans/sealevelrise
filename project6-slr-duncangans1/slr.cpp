
/* lidarview.cpp

  Duncan Gans (with much of the visualization code from Professor Toma)
  December 18
  GIS Algorithms and Data Structures

  This program takes a terrain grid and helps visualize sea level rise.
  Users can enter a certain amount of feet of sea level rise and the 
  funciton does two things. First, it creates a .asc file that represents
  the new terrain if the sea level rise happens. This can be rendered by
  a render2d function. More importantly, it creates a 3d rendering of the 
  terrain affected by sea level rise, with a visualization of where the
  water level would be. Users can use '+' and '-' to change the sea level
  as well as click '0' to go back to the original terrain without sea level
  rise. The green to red scale indicates the height of the land about sea level
  and the shades of blue represent different depths of water.

*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <math.h>
#include <assert.h>
//this allows this code to compile both on apple and linux platforms
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>
#include <queue>

using namespace std; 

//This queue represents the points that are between 0 and fIncerement feet 
//in elevation
queue<int>nextqueue;
//This vector is filled with the terrain grid.
vector<vector<float> > grid;
//Check Grid is a grid of ints that categorizes the points on the grid. 
//They are initially classified as ocean or land, but afterwards the land
//points are labeled as the necessary height for them to flood (in the 
//increment specified.)
vector<vector<float> > checkGrid;
//Grid that represents the completion of the grids.
vector<vector<int> > completionGrid;

//Necessary values. fIncrement is how specific the sea level rise by
//feet should be. Seavis indicates how much you should be able to 
//see below the surface (in feet).
int rows, cols, increment, seavis;
float maxz, feet, fIncrement, floorVal, ceiling, waterPoints, ndval;
float initLand = 0;

const int WINDOWSIZE = 500; 

//whenever the user rotates and translates the scene, we update these
//global translation and rotation
GLfloat pos[3] = {0,0,0};
GLfloat theta[3] = {0,0,0};

/* forward declarations of functions */
void display(void);
void keypress(unsigned char key, int x, int y);
void printDetails();

GLfloat xtoscreen(GLfloat x);
GLfloat ztoscreen(GLfloat z);
GLfloat ytoscreen(GLfloat y);

void readGridfromFile (char * filename)
{
  //Creates grid from a given file. It also initializes some variables
  FILE* f; 
  char s[100]; 

  f=fopen(filename, "r");
  if (f== NULL) {
     printf("cannot open files...");
     exit(1);
  }
  
  //Collecting the number of columns and rows for the grid details 
  fscanf(f, "%s", s);
  fscanf(f, "%d", &cols);
  fscanf(f, "%s", s);
  fscanf(f, "%d", &rows);

  //Creates empty checkGrid and normal grid
  for (int i = 0; i < rows; i++) {
    vector<float> row;
    for (int j = 0; j < cols; j++) {
      row.push_back(0);
    }
    checkGrid.push_back(row);
    grid.push_back(row);
    row.clear();
  }

  //Scans past unecessary information
  for (int i = 0; i < 4; i++)
  {
    fscanf(f, "%s", s); 
    //Here I am just using ndval to indicate the second int, the last round
    //it will actually set the value to the correct int
    fscanf(f, "%f", &ndval);
    //Continuing to move stuff into new file
  }
  printf("ndval = %f\n", ndval);
  printf("ROWS: %d, COLS: %d\n", rows, cols);

  //Puts all the values in the grid. And has checkgrid indicate 
  //possible ocean or land
  float newNum;
  for (int i = 0; i < rows * cols; i++)
  {
    fscanf(f, "%f", &newNum);
    grid[i/cols][i%cols] = newNum;
    if (newNum == ndval || newNum <= 0) {
      checkGrid[i/cols][i%cols] = -1;
    }
    else {
      initLand += 1;
      checkGrid[i/cols][i%cols] = 0;
    }
    if (newNum > maxz) {maxz = newNum;}
  }
}

void clearCompletion() {
  //Clears the completion grid for another flooding at a lower level
  completionGrid.clear();
  for (int i = 0; i < rows; i++) {
    vector<int> introw;
    for (int j = 0; j < cols; j++) {
     introw.push_back(0);
    }
    completionGrid.push_back(introw);

    introw.clear();
  }
}

void floodUp(queue<int> cqueue) {
  //This takes a queue of points that are on the coastline. It checks to see
  //if they are below the current flood height, and continue a breadth first
  //search to determine the rest of the points below the flood as well as
  //the coastline for the next iteration at the higher height.
  clock_t t3, t4;
  t3 = clock();
  queue<int> newqueue;
  if (feet >= ceiling) {return;}
  else {
	  while (!cqueue.empty()) {
	    int i = cqueue.front()/cols;
	    int j = cqueue.front()%cols;
	    cqueue.pop();
	    if (grid[i][j] <= feet) {
	      //If it's not ocean, but is floodable, set it to the current
	      //height, and add the points around it to the queue.
	      checkGrid[i][j] = feet;
	      if (j<cols-1) {if (completionGrid[i][j+1]==0){
	        completionGrid[i][j+1]=1;
	        cqueue.push(i*cols+(j+1));
	      }}
	      if (j>0) {if (completionGrid[i][j-1]==0){
	        completionGrid[i][j-1]=1;
	        cqueue.push(i*cols+(j-1));
	      }}
	      if (i<rows-1) {if (completionGrid[i+1][j]==0){
	        completionGrid[i+1][j]=1;
	        cqueue.push((i+1)*cols+j);
	      }}
	      if (i>0) {if (completionGrid[i-1][j]==0){
	        completionGrid[i-1][j]=1;
	        cqueue.push((i-1)*cols+j);
	      }}
	    }
      //If it's on the coastline, add it to the next coast
	    else {newqueue.push(i*cols+j);}
	}
  t4 = clock();
  printf("Flooding of %f feet takes %f seconds\n", 
    feet, (double)(t4-t3)/CLOCKS_PER_SEC);
  fflush(stdout);
	feet = feet+=fIncrement;
	floodUp(newqueue);
  }
}

void flood(int i, int j) {
  //This function floods the terrain grid from a given edge point
  //It creates a queue of ints that represent the coastline of the initial
  //ocean. This is used in subsequent foodUp()s to incrementally rise the 
  //sea level
  completionGrid[i][j] = 1;
  queue<int> myqueue;
  myqueue.push(i*cols+j);
  while (!myqueue.empty()) {
    //Initializes I and J from the queue's front point
    i = myqueue.front()/cols;
    j = myqueue.front()%cols;
    myqueue.pop();
    if (checkGrid[i][j] == -1 || grid[i][j] <= 0) {
      //If it's ocean, add untouched points around it to the grid
      if (j<cols-1) {if (completionGrid[i][j+1]==0){
        completionGrid[i][j+1]=1;
        myqueue.push(i*cols+(j+1));
      }}
      if (j>0) {if (completionGrid[i][j-1]==0){
      completionGrid[i][j-1]=1;
      myqueue.push(i*cols+(j-1));
      }}
      if (i<rows-1) {if (completionGrid[i+1][j]==0){
        completionGrid[i+1][j]=1;
        myqueue.push((i+1)*cols+j);
      }}
      if (i>0) {if (completionGrid[i-1][j]==0){
        completionGrid[i-1][j]=1;
        myqueue.push((i-1)*cols+j);
      }}
    }
    else if (grid[i][j] <= 0 && checkGrid[i][j] != -1) {
      //If it's not ocean, but is floodable, set it to the current
      //height, and add the points around it to the queue.
      checkGrid[i][j] = 0;
      if (j<cols-1) {if (completionGrid[i][j+1]==0){
        completionGrid[i][j+1]=1;
        myqueue.push(i*cols+(j+1));
      }}
      if (j>0) {if (completionGrid[i][j-1]==0){
        completionGrid[i][j-1]=1;
        myqueue.push(i*cols+(j-1));
      }}
      if (i<rows-1) {if (completionGrid[i+1][j]==0){
        completionGrid[i+1][j]=1;
        myqueue.push((i+1)*cols+j);
      }}
      if (i>0) {if (completionGrid[i-1][j]==0){
        completionGrid[i-1][j]=1;
        myqueue.push((i-1)*cols+j);
      }}
    }
    else {nextqueue.push(i*cols+j);}
  }
}



void slr() {
  //Go around border and if there's a point that is ocean, flood from
  //that point if it hasn't been touched yet.
  clock_t t3, t4;
  t3 = clock();
  for (int j = 0; j < cols; j++) {
    if (checkGrid[0][j] == -1 && !completionGrid[0][j]) {
      flood(0, j);
    }
    if (checkGrid[rows-1][j] == -1 && !completionGrid[rows-1][j]) {
      flood(rows-1, j);
    }
  }
  for (int i = 0; i < rows; i++) {
    if (checkGrid[i][0] == -1 && !completionGrid[i][0]) {
      flood(i, 0);
    }
    if (checkGrid[i][cols-1] == -1 && !completionGrid[i][cols-1]) {
      flood(i, cols-1);
    }
  }
  feet = feet += fIncrement;
  t4 = clock();
  printf("Finding ocean takes %f seconds\n", (double)(t4-t3)/CLOCKS_PER_SEC);
  //Now that all the coast points are added to nextqueue, run the incremental
  //flood algorithms on the coasline
  floodUp(nextqueue);
}

void getColor(float height, int type) {
  //This determines the color at an individual point.
  float margin = height - feet;
  if (type == 0 || type >= feet) {
    //If it's above ground, change the color depending on the 
    //elevation. This will create a topographical look.
    if (margin < -maxz/30) {glColor3f(.9, .0, .0);}
    else if (margin < 0) {glColor3f(.0, .2, .0);} 
    else if (margin < maxz/70) {glColor3f(.1, .3, .2);}
    else if (margin < maxz/45) {glColor3f(.1, .4, .3);}
    else if (margin < maxz/25) {glColor3f(.1, .4, .3);}
    else if (margin < maxz/10) {glColor3f(.2, .5, .4);}
    else if (margin < 2*maxz/10) {glColor3f(.4, .6, .3);}
    else if (margin < 3*maxz/10) {glColor3f(.6, .8, .3);}
    else if (margin < 4*maxz/10) {glColor3f(.8, .9, .2);}
    else if (margin < 5*maxz/10) {glColor3f(1, 1, .2);}
    else if (margin < 6*maxz/10) {glColor3f(.9, .6, .2);}
    else if (margin < 7*maxz/10) {glColor3f(.7, .4, .2);}
    else if (margin < 8*maxz/10) {glColor3f(.6, .3, .1);}
    else if (margin < 9*maxz/10) {glColor3f(.4, .1, .1);}
    else {glColor3f(.3, .2, .1);}
  }
  //Otherwise, set the initial water to a deepish blue, and the
  //flooded water zones to a scaled blue color to represent the
  //Depth. This uses seavis as an option to determine how much
  //below sea level should be visible. This can be changed at the
  //command line.
  else if (type == -1) {glColor3f(.1, .1, .9);}
  else {
    glColor3f(.1, (height+seavis - feet)/seavis - .1, .8);
    waterPoints+=(increment * increment);
  }
} 

void createTriangles(int i, int j) {
  //Creates two triangles at a given I and J. Gets the color using get
  //color and sets the height either to the height of the ocean, or the
  //height of the land. Pretty straightforward.
  getColor(grid[i][j], checkGrid[i][j]);
  if (checkGrid[i][j] == 0 || checkGrid[i][j] > feet) {
    glBegin(GL_POLYGON);
    glVertex3f(ytoscreen(i), xtoscreen(j), ztoscreen(grid[i][j]));
    glVertex3f(ytoscreen(i), xtoscreen(j+increment), ztoscreen(grid[i][j+increment]));
    glVertex3f(ytoscreen(i+increment), xtoscreen(j), ztoscreen(grid[i+increment][j]));
    glEnd();
    glBegin(GL_POLYGON);
    glVertex3f(ytoscreen(i), xtoscreen(j), ztoscreen(grid[i][j]));
    glVertex3f(ytoscreen(i-increment), xtoscreen(j), ztoscreen(grid[i-increment][j]));
    glVertex3f(ytoscreen(i), xtoscreen(j-increment), ztoscreen(grid[i][j-increment]));
    glEnd();
  }
  else {
    glBegin(GL_POLYGON);
    glVertex3f(ytoscreen(i), xtoscreen(j), ztoscreen(feet));
    glVertex3f(ytoscreen(i), xtoscreen(j+increment), ztoscreen(feet));
    glVertex3f(ytoscreen(i+increment), xtoscreen(j), ztoscreen(feet));
    glEnd();
    glBegin(GL_POLYGON);
    glVertex3f(ytoscreen(i), xtoscreen(j), ztoscreen(feet));
    glVertex3f(ytoscreen(i-increment), xtoscreen(j), ztoscreen(feet));
    glVertex3f(ytoscreen(i), xtoscreen(j-increment), ztoscreen(feet));
    glEnd();
  }
}

void visGrid(){
  waterPoints = 0;
  //This function visualizes all of the grid. It does it by doing two
  //triangles at every point. One that goes from a point to [i+1][j] 
  //and [i][j+1], and another that goes from a point to [i-1][j] to
  //[i][j-1]
  for (int i = increment; i < rows - increment - 1; i+=increment)
  {
    for (int j = increment; j < cols - increment - 1; j+=increment)
    {
      createTriangles(i, j);
    }
  }
  printDetails();
  glEnd();
}


void moveToFile(char * newfile, float initHeight) {
  //Moves the grid into a renderable file. 
  FILE*n;
  n = fopen(newfile, "w");
  //Fills in default info at start. For stuff that the value doesn't
  //mattter, it is just set to ndval.
  fprintf(n, "%s", "ncols");
  fprintf(n, "%d\n", cols);
  fprintf(n, "%s", "nrows");
  fprintf(n, "%d\n", rows);
  fprintf(n, "%s", "xllcorner");
  fprintf(n, "%f\n", ndval);
  fprintf(n, "%s", "yllcorner");
  fprintf(n, "%f\n", ndval);
  fprintf(n, "%s", "cellsize");
  fprintf(n, "%f\n", ndval);
  fprintf(n, "%s", "NODATA_value");
  fprintf(n, "%f\n", ndval);
  //Moves data from grid into the file.
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (checkGrid[row][col] >= initHeight || checkGrid[row][col] == 0) {
        fprintf(n, "%f ", grid[row][col] - initHeight);
      }
      else {fprintf(n, "%d ", 0);}
    }
  }
}

int main(int argc, char** argv) {
  //This file does all the heavy lifting described at the top of the code
  //What essentially happens is that the user denotes a maximum height, and 
  //the function begins at zero, and slowly rises the sea level by the increment
  //given, by having a queue that keeps track of the current elements at or above
  //the given height. Those that are over the current flood height (on the coast)
  // get put in a stack for the next iteration with the feet a little higher. 
  //To do this, it is constantly keeping track of the coastline.

  //read number of points from user
  if (argc<5 || argc>6) {
    printf("usage: %s file.txt file.txt height range increment\n", argv[0]);
    exit(1); 
  }

  //If entered, seavis is set to the value. Otherwise is 10 feet
  if (argc<6) {seavis = 10;}
  else {seavis = atoi(argv[5]);}

  //Grid is red, and variables are read from command line.
  printf("Has begun reading file\n");
  clock_t t1, t2;
  t1 = clock();
  readGridfromFile(argv[1]); 
  t2 = clock();
  printf("Finished reading file\n");
  printf("Reading file takes %f seconds\n", (double)(t2-t1)/CLOCKS_PER_SEC);
  fIncrement = atof(argv[4]);
  ceiling = atof(argv[3]);

  //Although the precision of the grid is always 1. The visualization
  //resolution changes depending on the size of the grid. This makes it
  //so you can change the sea level, or rotate the grid without having
  //to wait.
  if ((rows+cols)/2 < 1000) {increment = 1;}
  else if ((rows+cols)/2 < 5000) {increment = 5;}
  else if ((rows+cols)/2 < 15000) {increment = 20;}
  else if ((rows+cols)/2 < 25000) {increment = 50;}
  else {increment = 100;}


  feet = floorVal;
  clearCompletion();
  t1 = clock();
  slr();
  t2 = clock();
  printf("Running %f iterations of sea level rise takes %f seconds\n", 
  	ceiling/fIncrement, (double)(t2-t1)/CLOCKS_PER_SEC);
  printf("Or approximately %f seconds per iteration\n", 
  	((double)(t2-t1)/CLOCKS_PER_SEC)/ceiling);
  fflush(stdout);
  //Floods it at 0, so users can see what it looks like with no flooding.
  moveToFile(argv[2], ceiling);
  /* OPEN GL STUFF */
  /* open a window and initialize GLUT stuff */
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(WINDOWSIZE, WINDOWSIZE);
  glutInitWindowPosition(100,100);
  glutCreateWindow(argv[0]);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* register callback functions */
  glutDisplayFunc(display); 
  glutKeyboardFunc(keypress);
  
  /* OpenGL init */
  /* set background color black*/
  glClearColor(0, 0, 0, 0);  
  glEnable(GL_DEPTH_TEST); 

  /* setup the camera (i.e. the projection transformation) */ 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, 1 /* aspect */, 1, 10.0);
  /* camera is at (0,0,0) looking along negative z axis */
  
  //initialize the translation to bring the points in the view frustrum which is [-1, -10]
  pos[2] = -2;

  //initialize rotation to look at it from above 
  theta[0] = -45; 
  visGrid();
  /* start the event handler */
  glutMainLoop();

  return 0;
}

void printDetails() {
  printf("At %f feet:\n", feet);
  printf("Aproximately %f percent of the original land is flooded\n",
   (waterPoints/initLand)*100);
}

/* this function is called whenever the window needs to be rendered */
void display(void) {

  //clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //clear all modeling transformations 
  glMatrixMode(GL_MODELVIEW); 
  glLoadIdentity();

  /* The default GL window is x=[-1,1], y= [-1,1] with the origin in
     the center.  The view frustrum was set up from z=-1 to z=-10. The
     camera is at (0,0,0) looking along negative z axis.
  */ 

 /*  we translate and rotate our local reference system with the
    user transformation. pos[] represents the cumulative translation
    entered by the user, and theta[] the cumulative rotation entered
    by the user */
  glTranslatef(pos[0], pos[1], pos[2]);  
  glRotatef(theta[0], 1,0,0); //rotate theta[0] around x-axis, etc 
  glRotatef(theta[1], 0,1,0);
  glRotatef(theta[2], 0,0,1);
  
  //Visualizes the current grid
  visGrid();
  glFlush();
}




/* this function is called whenever  key is pressed */
void keypress(unsigned char key, int x, int y) {

  switch(key) {
  case '+':
    //Raises sea level rise by one foot increment (can be changed)
    if (feet + fIncrement <= ceiling) {feet += fIncrement;}
    glutPostRedisplay();
    break;
  case '-':
    //Lowers sea level rise by one foot increment (can be changed)
    if (feet - fIncrement >= floorVal) {feet -= fIncrement;}
    glutPostRedisplay();
    break;
  case '0':
    //if (feet) {feet = 0;}
    //else {feet = checkHeight;}
    glutPostRedisplay();
    break;
  case '2': 
    //3d orthogonal projection, view from straight above
    glMatrixMode(GL_PROJECTION);
    //the view frustrum is z=[0, -20]
    glOrtho(-1, 1, -1, 1, 0,-20); //left, right, top, bottom, near, far
    
    //initial view is from (0,0,-5) ie above the terrain looking straight down
    pos[0]=pos[1]=0; pos[2] = -7; 
    //initial view: no rotation
    theta[0]=theta[1] = theta[2]= 0; 
    glutPostRedisplay();
    break;
  
  case '3': 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1 /* aspect */, 1, 10.0); /* the frustrum is from z=-1 to z=-10 */
    /* camera is at (0,0,0) looking along negative z axis */
    //initialize the translation; view frustrum is z= [-1, -10] and
    //initial position (0,0,-2)
    pos[0]=pos[1]=0; pos[2] = -2;
    //initialize rotation to look  from above 
    theta[1] = theta[2] = 0;  theta[0] = -45; 
    glutPostRedisplay();
    break;

    //ROTATIONS 
  case 'x':
    theta[0] += 5.0; 
    glutPostRedisplay();
    break;
  case 'y':
    theta[1] += 5.0;
    glutPostRedisplay();
    break;
  case 'z':
    theta[2] += 5.0;
    glutPostRedisplay();
    break;
  case 'X':
    theta[0] -= 5.0; 
    glutPostRedisplay();
    break;
  case 'Y':
    theta[1] -= 5.0; 
    glutPostRedisplay();
    break;
  case 'Z':
    theta[2] -= 5.0; 
    glutPostRedisplay();
    break;
    
    //TRANSLATIONS 
    //backward (zoom out)
  case 'b':
    pos[2] -= 0.1; 
    glutPostRedisplay();
    break;
    //forward (zoom in)
  case 'f':
    pos[2] += 0.1; 
    //glTranslatef(0,0, 0.5);
    glutPostRedisplay();
    break;
    //down 
  case 'd': 
     pos[1] -= 0.1; 
    //glTranslatef(0,0.5,0);
    glutPostRedisplay();
    break;
    //up
  case 'u': 
    pos[1] += 0.1; 
    //glTranslatef(0,-0.5,0);
    glutPostRedisplay();
    break;
    //left 
  case 'l':
    pos[0] -= 0.1; 
    glutPostRedisplay();
    break;
    //right
  case 'r':
    pos[0] += 0.1; 
    glutPostRedisplay();
    break;
  case 'q':
    exit(0);
    break;
  } 
}//keypress

GLfloat xtoscreen(GLfloat x) {
  //return (-1 + 2*x/WINDOWSIZE); 
  //printf("X: %f", -1 + 2*(x)/(cols));
  return -1 + 2*(x)/(double)cols;
}

/* y is a value in [miny, maxy]; it is mapped to [-1,1] */
GLfloat ytoscreen(GLfloat y) {
  return -1 + 2*(y/(double)rows);
}

/* z is a value in [minz, maxz]; it is mapped so that [0, maxz] map to [0,1] */
GLfloat ztoscreen(GLfloat z) {
	if(z < -99) {return 0;}
	return (z/maxz)/5;
}


