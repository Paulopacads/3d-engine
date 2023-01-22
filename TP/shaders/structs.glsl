struct CameraData {
    mat4 view_proj;
};

struct FrameData {
    CameraData camera;

    mat4 sun_view_proj;

    vec3 sun_dir;
    uint point_light_count;

    vec3 sun_color;
    float padding_1;
};

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float padding_1;
};

struct Model {
    mat4 transform;
};
