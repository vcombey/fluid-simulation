
#include "physician.hpp"

#include <ctime>

using namespace mod1;

Physician::Physician(){};

Physician::~Physician() {}



/*
 * function hat(r) = 1 - r if ( 0 <= r <= 1)
 *                  1 + r if ( -1 <= r <= 0)
 *                  0 else
 */

double Physician::hat(double r) {
    if (0 <= r && r <= 1)
        return (1 - r);
    else if (-1 <= r && r <= 0)
        return (1 + r);
    else
    {
        //std::cerr << "bad input for function hat ?" << std::endl;
        return 0;
    }
}

/*
 * function b_spline(r) =
 *      1 / 2 (r + 3 / 2) ^ 2 if -3 / 2 <= r < -1 / 2
 *      3 / 4 - r^2           if -1 / 2 <= r < 1 / 2
 *      1 / 2 (3 / 2 - r) ^ 2 if 1 / 2 <= r < 3 / 2
 *      0                     else
 */

double Physician::b_spline(double r) {
    if (-1.5 <= r && r <= -0.5)
        return (0.5 * (r + 1.5) * (r + 1.5));
    else if (-0.5 <= r && r <= 0.5)
        return (0.75 - r * r);
    else if (0.5 <= r && r <= 1.5)
        return (0.5 * (1.5 - r) * (1.5 - r));
    else
    {
        // std::cerr << "bad input for function b_spline ?" << std::endl;
        return 0;
    }
}

/*
 * function kernel(x,y,z) = hat(x/DX)*hat(y/DY)*hat(z/DZ)
 */

double Physician::kernel(double x, double y) {
    return (b_spline(x / DX) * b_spline(y / DY));
}
/*
 * m_grid[i][j] is the case where the particle is
 * so the particle velocity contribute to
 * m_grid_u[i][j] and m_grid_v[i + 1][j]
 *
 *             j + 1  _______________
 *                    |             |
 *                    |  .          |
 *      j + 0.5 * DY  |.   i,j      |
 *                    |             |
 *                    |    .        |
 *              j     ---------------
 *                   i *dx         (i + 1)*dx
 *
 *  the distance for the velocity grid :
 *  grid_u[i][j] = (i * dx , j * dy)
 *  the distance for the pressure grid :
 *  grid_p[i][j] = ((i + 0.5) * dx , (j + 0.5) * dy)
 *
 *  m_grid_u[i][j] = ∑ (k(xp - i * dx)up / ∑(k(xp - i * dx)
 *
 *
 */

void Physician::evaluateGridComponentVelocity(vector3d position,
                                              vector3d velocity,
                                              vector3d gridOffset,
                                              char field)
{
    position -= gridOffset;
    int gi = position.x / DX;
    int gj = position.y / DY;

    for (int j = 0; j < 4; j++) {
        int grid_j = j + gj - 1;
        for (int i = 0; i < 4; i++) {
            int grid_i = i + gi - 1;
            double kernelVal = kernel(position.x - grid_i * DX, position.y - grid_j * DY);
            if (field == 'u') {
                GRID_U[grid_i][grid_j].sum += kernelVal * velocity.x;
                GRID_U[grid_i][grid_j].weight += kernelVal;
            if (kernelVal > 0 && GRID[grid_i][grid_j].type == AIR)
                GRID[grid_i][grid_j].type = FLUID;
            }
            else {
                GRID_V[grid_i][grid_j].sum += kernelVal * velocity.y;
                GRID_V[grid_i][grid_j].weight += kernelVal;
            if (kernelVal > 0 && GRID[grid_i][grid_j].type == AIR)
                GRID[grid_i][grid_j].type = FLUID;
            }
        }
    }
}

void Physician::evaluateGridVelocityForParticle(vector3d position, vector3d velocity) {
    double hdx = 0.5 * DX;
    vector3d offsetU(0.0, hdx, hdx);
    vector3d offsetV(hdx, 0.0, hdx);
    //vector3d offsetW(hdx, hdx, 0.0);

    evaluateGridComponentVelocity(position, velocity, offsetU, 'u');
    evaluateGridComponentVelocity(position, velocity, offsetV, 'v');
}

void Physician::put_velocity_on_grid() {
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            GRID_U[i][j].sum = 0;
            GRID_V[i][j].sum = 0;
            GRID_U[i][j].weight = 0;
            GRID_V[i][j].weight = 0;
            GRID_U[i][j].oldVal = 0;
            GRID_V[i][j].oldVal = 0;
            GRID_V[i][j].val = 0;
            GRID_U[i][j].val = 0;
            if (GRID[i][j].type == FLUID)
                GRID[i][j].type = AIR;
        }
    }
    //vector3d offsetW(hdx, hdx, 0.0);
    for (unsigned long int p = 0; p < PARTICLES.size(); p++) {
        /*
         * m_grid[i][j] is the case where the particle is
         * so the particle velocity contribute to
         * m_grid_u[i][j] and m_grid_v[i + 1][j]
         *
         *             j + 1  _______________
         *                    |             |
         *                    |  .          |
         *      j + 0.5 * DY  |.   i,j      |
         *                    |             |
         *                    |    .        |
         *              j     ---------------
         *                   i *dx         (i + 1)*dx
         *
         *  the distance for the velocity grid :
         *  grid_u[i][j] = (i * dx , j * dy)
         *  the distance for the pressure grid :
         *  grid_p[i][j] = ((i + 0.5) * dx , (j + 0.5) * dy)
         *
         *  m_grid_u[i][j] = ∑ (k(xp - i * dx)up / ∑(k(xp - i * dx)
         *
         *
         */
        int i = PARTICLES[p].pos.x / DX;
        int j = PARTICLES[p].pos.y / DY;
        if (GRID[i][j].type == AIR)
            GRID[i][j].type = FLUID; 
        evaluateGridVelocityForParticle(PARTICLES[p].pos, PARTICLES[p].vel);
        /*
           vector3d position = PARTICLES[p].pos;
           position -= gridOffset;
           int gi = position.x / DX;
           int gj = position.y / DX;
           double x = PARTICLES[p].pos.x;
           double y = PARTICLES[p].pos.y;
           double up = PARTICLES[p].vel.x;
           double vp = PARTICLES[p].vel.y;
        //std::cout << "x"<<x <<"y" << y << "velx"<< up << "vely" << vp <<std::endl;
        int i = x / DX;
        int j = y / DY;
        if (i < 0 || i >= GRID_WIDTH)
        continue;
        if (j < 0 || j >= GRID_HEIGHT)
        continue;
        if (GRID[i][j].type == AIR)
        GRID[i][j].type = FLUID;
        //TODO: update also m_grid_u[i - 1]

        GRID_U[i][j].sum        += kernel(x - i * DX, y - (j + 0.5) * DY) * up;
        GRID_U[i + 1][j].sum    += kernel(x - (i + 1) * DX, y - (j + 0.5) * DY) * up;

        GRID_V[i][j].sum        += kernel(x - (i + 0.5) * DX, y - j * DY) * vp;
        GRID_V[i][j + 1].sum    += kernel(x - (i + 0.5) * DX, y - (j + 1) * DY) * vp;

        GRID_U[i][j].weight     += kernel(x - i * DX, y - (j + 0.5) * DY);
        GRID_U[i + 1][j].weight += kernel(x - (i + 1) * DX, y - (j + 0.5) * DY);

        GRID_V[i][j].weight     += kernel(x - (i + 0.5) * DX, y - j * DY);
        GRID_V[i][j + 1].weight += kernel(x - (i + 0.5) * DX, y - (j + 1) * DY);
*/
        }
    for (int i = 0; i < GRID_WIDTH + 1; i++) {
        for (int j = 0; j < GRID_HEIGHT + 1; j++) {
            if (j < GRID_HEIGHT && GRID_U[i][j].weight) {
                GRID_U[i][j].val = GRID_U[i][j].sum / GRID_U[i][j].weight;
                GRID_U[i][j].oldVal = GRID_U[i][j].val;
            }
            if (i < GRID_WIDTH && GRID_V[i][j].weight) {
                GRID_V[i][j].val = GRID_V[i][j].sum / GRID_V[i][j].weight;
                GRID_V[i][j].oldVal = GRID_V[i][j].val;
            }
        }
    }
}

double Physician::cubicInterpolate(double p[4], double x) {
    return p[1] + 0.5 * x*(p[2] - p[0] + 
            x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + 
                x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

double Physician::bicubicInterpolate(double p[4][4], double x, double y) {
    double arr[4];
    arr[0] = cubicInterpolate(p[0], x);
    arr[1] = cubicInterpolate(p[1], x);
    arr[2] = cubicInterpolate(p[2], x);
    arr[3] = cubicInterpolate(p[3], x);
    return cubicInterpolate(arr, y);
}

double Physician::evaluateComponentVelocity(vector3d position,
        vector3d gridOffset,
        char field, char method)
{
    position -= gridOffset;
    int gi = position.x / DX;
    int gj = position.y / DX;
    double points[4][4];

    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            if (field == 'u')
            {
                if (method == 'p') /* PIC */
                    points[j][i] = GRID_U[i + gi - 1][j + gj - 1].val;
                else              /* FLIP */
                    points[j][i] = GRID_U[i + gi - 1][j + gj - 1].val - GRID_U[i + gi - 1][j + gj - 1].oldVal;
            }
            else
            {
                if (method == 'p') 
                    points[j][i] = GRID_V[i + gi - 1][j + gj - 1].val;
                else
                    points[j][i] = GRID_V[i + gi - 1][j + gj - 1].val - GRID_V[i + gi - 1][j + gj - 1].oldVal;
            }
        }
    }
    vector3d gpos = vector3d(gi, gj, 0) * DX;
    vector3d interp = (position - gpos) / DX;
    return bicubicInterpolate(points, interp.x, interp.y);
}

vector3d Physician::evaluateVelocityAtPosition(vector3d position, char method) {
    double hdx = 0.5 * DX;
    vector3d offsetU(0.0, hdx, hdx);
    vector3d offsetV(hdx, 0.0, hdx);
    //vector3d offsetW(hdx, hdx, 0.0);

    double vx = evaluateComponentVelocity(position, offsetU, 'u', method);
    double vy = evaluateComponentVelocity(position, offsetV, 'v', method);
    return vector3d(vx, vy, 0);
}

void Physician::get_velocity_from_the_grid() {
    for (unsigned long int p = 0; p < PARTICLES.size(); p++) {
        /*
         *             j + 1  _______________
         *                    |             |
         *                    |  .          |
         *      j + 0.5 * DY  |.   i,j      |
         *                    |             |
         *                    |    .        |
         *              j     ---------------
         *                   i *dx         (i + 1)*dx
         */
        vector3d flip_component = PARTICLES[p].vel + evaluateVelocityAtPosition(PARTICLES[p].pos, 'f');
        PARTICLES[p].vel = evaluateVelocityAtPosition(PARTICLES[p].pos, 'p') * PIC
            + ( flip_component * FLIP);
    }
}

uint32_t Physician::init_particules(uint32_t ox, uint32_t oy, uint32_t width, uint32_t height, bool randomize) {
    if (ox + width >= GRID_WIDTH || oy + height >= GRID_HEIGHT) {
        std::cerr << "Some particles will be outside the grid: " << ox + width << " " << oy + height << " on " << GRID_WIDTH << "*" << GRID_HEIGHT << std::endl;
        return 0;
    }
    uint32_t nb_particles = width * height * DENSITY_RACINE * DENSITY_RACINE;
    if (nb_particles == 0) {
        std::cerr << "There are no particle to fill it" << std::endl;
        return 0;
    }
    std::srand(std::time(0));
    size_t offset = PARTICLES.size();
    PARTICLES.resize(PARTICLES.size() + nb_particles);
    for (unsigned long int i = 0; i < nb_particles; i++) {
        double a = ox;
        double b = oy;
        if (randomize) {
            a += (double)std::rand() / RAND_MAX * DX / DENSITY_RACINE;
            b += (double)std::rand() / RAND_MAX * DY / DENSITY_RACINE;
        }
        PARTICLES[offset + i].pos.x = (a + ((double)(i % (width * DENSITY_RACINE)) / DENSITY_RACINE)) * DX;
        PARTICLES[offset + i].pos.y = (b + ((double)(i / (width * DENSITY_RACINE)) / DENSITY_RACINE)) * DY;
        //PARTICLES[i].vel = 0;
    }
    return nb_particles;
}

void Physician::advect() {

    for (unsigned long int p = 0; p < PARTICLES.size(); p++) {
        PARTICLES[p].pos += PARTICLES[p].vel * DT;
    }

    /* hack pour que les particules ne rentre pas dans le mur */
    /*
       for (int p = 0; p < NB_PARTICLES; p++) {
       double new_x;
       double new_y;
       new_x = PARTICLES[p].x + PARTICLES[p].u * DT;
       new_y = PARTICLES[p].y + PARTICLES[p].v * DT;

       int i = new_x / DX;
       int j = new_y / DY;

       if (i < 0 || i >= GRID_WIDTH || j < 0 || j >= GRID_HEIGHT || GRID[i][j].type == SOLID)
       continue;
       if (new_x / DX) {
       PARTICLES[p].x = new_x;
       PARTICLES[p].y = new_y;
       }
       }
       */
}
