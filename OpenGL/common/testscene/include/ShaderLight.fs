R"fs(
    #version 300 es
    precision mediump float;

    uniform vec3 u_lightPosition;
    uniform vec3 u_camera;

    in vec3 _FragPos;
    in vec3 _Normal;

    out vec4 outColor;

    void main()
    {
        vec3 normal = normalize(_Normal);

        vec3 color =  vec3(1.0, 0.0, 0.0);

        vec3 lightDir = normalize(u_lightPosition - _FragPos);
        float diff = max(dot(lightDir, normal), 0.1);
        float distance = length(u_lightPosition - _FragPos);
        diff = diff / (0.45 * distance * distance);
        vec3 diffuse = color * diff;

        vec3 viewDir = normalize(u_camera - _FragPos);
        vec3 reflectDir = reflect(-viewDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128.0);
        vec3 specular =  vec3(1.0, 0.0, 0.0) * spec * 0.5;

        vec4 result = vec4(diffuse + specular, 1.0);
        outColor = result;
    }
)fs"