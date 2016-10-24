/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "World.h"
#include <limits>
#include "Vector3D.h"
#include "Point3D.h"
#include "Normal.h"
#include "Ray.h"
#include "Sphere.h"
#include "Plane.h"
#include "Regular.h"
#include <fstream>

World::World(){
    m_vp = ViewPlane(200, 200, 1.0, 1.0);
    m_objects = std::vector<GeometryObject*>(0);
    m_pixels = std::vector<RGBColor>(m_vp.width * m_vp.height);
}

World::~World(){
    delete m_tracer;
}

void World::add_object(GeometryObject* obj){
    m_objects.push_back(obj);
}

void World::build(){
    m_tracer = new MultiTracer(this);
    m_vp.set_sampler(new Regular(25, 1));
    
    Sphere* red_sphere = new Sphere(Point3D(0,0,0), 85);
    red_sphere->setColor(RGBColor(1,0,0));
    add_object(red_sphere);
    /*
    Sphere* yellow_sphere = new Sphere(Point3D(0,30,0), 60);
    yellow_sphere->setColor(RGBColor(1,1,0));
    add_object(yellow_sphere);

    Plane* green_plane = new Plane(Point3D(0,0,0), Normal(0,1,1));
    green_plane->setColor(RGBColor(0,1,0));
    add_object(green_plane);
    */
}

void World::render_scene() {
    Ray ray;
    
    ray.d = Vector3D(0, 0, -1);
    
    for (int x = 0; x < m_vp.width; ++x){
        for (int y = 0; y < m_vp.height; ++y){
            RGBColor pixel_color;
            for (int s = 0; s < m_vp.get_n_samples(); ++s){
                const Point2D sample = m_vp.sampler_ptr->next_sample();
                
                const double world_x = m_vp.pixel_size * (x - 0.5 * (m_vp.width + sample.x));
                const double world_y = m_vp.pixel_size * (y - 0.5 * (m_vp.height + sample.y));

                ray.o = Point3D(world_x, world_y, 100);
                pixel_color += m_tracer->trace_ray(ray);
            }
            pixel_color /= m_vp.get_n_samples();
            display_pixel(x, y, pixel_color);
        }
    }
}
#include <iostream>

const ShadeRec World::hit_bare_bones_obj(const Ray& ray) const {
    ShadeRec sr_min, sr;
    
    sr.hit = false;
    sr.color = RGBColor();
    sr.hit_point = Point3D();
    sr.local_hit_point = Point3D();    
    sr.hit_normal = Normal();
    sr.ray = ray;
    //sr.world = *this;

    double t, t_min = std::numeric_limits<float>::max();
    for(const auto& obj : m_objects){
        bool hit = obj->hit(ray, t, sr);
        if (obj->hit(ray, t, sr) && (t < t_min)){
            t_min = t;
            sr_min = sr;
        }
    }
    return sr_min;
}

void World::display_pixel(const int x, const int y, const RGBColor& pixel_color) {
    int new_index = (y * m_vp.width) + x;
    m_pixels[new_index] = pixel_color;
}

void World::save_image(const std::string& outputFile) const {
    const int image_size = m_vp.height * m_vp.width * 4;
    const int headers_size = 14 + 40;
    const int filesize = image_size + headers_size;
    const int pixelsPerMeter = 2835;
    
    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0};
    //size of the file in bytes
    bmpfileheader[ 2] = (unsigned char)(filesize);
    bmpfileheader[ 3] = (unsigned char)(filesize>>8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);
            
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0};
    //width of the image in bytes
    bmpinfoheader[ 4] = (unsigned char)(m_vp.width);
    bmpinfoheader[ 5] = (unsigned char)(m_vp.width>>8);
    bmpinfoheader[ 6] = (unsigned char)(m_vp.width>>16);
    bmpinfoheader[ 7] = (unsigned char)(m_vp.width>>24);
    
    //height of the image in bytes
    bmpinfoheader[ 8] = (unsigned char)(m_vp.height);
    bmpinfoheader[ 9] = (unsigned char)(m_vp.height>>8);
    bmpinfoheader[10] = (unsigned char)(m_vp.height>>16);
    bmpinfoheader[11] = (unsigned char)(m_vp.height>>24);

    // Size image in bytes
    bmpinfoheader[21] = (unsigned char)(image_size);
    bmpinfoheader[22] = (unsigned char)(image_size>>8);
    bmpinfoheader[23] = (unsigned char)(image_size>>16);
    bmpinfoheader[24] = (unsigned char)(image_size>>24);

    bmpinfoheader[25] = (unsigned char)(pixelsPerMeter);
    bmpinfoheader[26] = (unsigned char)(pixelsPerMeter>>8);
    bmpinfoheader[27] = (unsigned char)(pixelsPerMeter>>16);
    bmpinfoheader[28] = (unsigned char)(pixelsPerMeter>>24);

    bmpinfoheader[29] = (unsigned char)(pixelsPerMeter);
    bmpinfoheader[30] = (unsigned char)(pixelsPerMeter>>8);
    bmpinfoheader[31] = (unsigned char)(pixelsPerMeter>>16);
    bmpinfoheader[32] = (unsigned char)(pixelsPerMeter>>24);

    FILE *file = fopen(outputFile.c_str(), "wb");//write-binary
    
    fwrite(bmpfileheader,1,14, file);
    fwrite(bmpinfoheader,1,40, file);
    
    for (int i = 0; i < m_pixels.size(); ++i){
        const RGBColor pixel = m_pixels[i];
        unsigned char color[3] = {
            (int) pixel.b * 255, 
            (int) pixel.g * 255, 
            (int) pixel.r * 255
        };
        fwrite(color, 1, 3, file);
    }
    fclose(file);
}
