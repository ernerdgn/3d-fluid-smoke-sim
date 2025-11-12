#include "GpuGrid3D.h"
#include <iostream>
#include <vector>

GLuint GpuGrid3D::create3DTexture(int internal_format, int format)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // alloc, fill nullptr
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, m_width, m_height, m_depth,
        0, format, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return textureID;
}

GpuGrid3D::GpuGrid3D(int width, int height, int depth)
    : m_width(width), m_height(height), m_depth(depth)
{
    // fbo
    glGenFramebuffers(1, &m_fbo);

    // 3d texture

    // density
    m_densityTexA = create3DTexture(GL_R32F, GL_RED);
    m_densityTexB = create3DTexture(GL_R32F, GL_RED);

    // velo
    m_velocityTexA = create3DTexture(GL_RGB32F, GL_RGB);
    m_velocityTexB = create3DTexture(GL_RGB32F, GL_RGB);
}

GpuGrid3D::~GpuGrid3D()
{
    glDeleteFramebuffers(1, &m_fbo);

    GLuint textures[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB
    };
    glDeleteTextures(4, textures);
}

void GpuGrid3D::clear(Shader& clearShader)
{
    clearShader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    // clear
    GLuint texturesToClear[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB
        // new textures will be added here
    };

    for (GLuint tex : texturesToClear)
    {
        // Loop through all slices
        for (int z = 0; z < m_depth; ++z)
        {
            // z->fbo
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                tex, 0, z);

            // set fbo
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            // run clear shadeer
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GpuGrid3D::swapDensityBuffers()
{
    std::swap(m_densityTexA, m_densityTexB);
}

void GpuGrid3D::swapVelocityBuffers()
{
    std::swap(m_velocityTexA, m_velocityTexB);
}

void GpuGrid3D::step(Shader& splatShader, Shader& advectShader, Shader& diffuseShader,
    const glm::vec3& mouse_pos3D, const glm::vec3& mouse_vel,
    bool is_bouncing, float delta_time, float viscosity, int diffuse_iterations)
{
    splatShader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    float brush_radius = m_width * 0.025f; // 3D brush

    // set uniforms
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_brush_center3D"), 1, glm::value_ptr(mouse_pos3D));  //glm::value_ptr(mouse_pos3D)
    glUniform1f(glGetUniformLocation(splatShader.ID, "u_radius"), brush_radius);
    glUniform1i(glGetUniformLocation(splatShader.ID, "u_is_bouncing"), is_bouncing ? 1 : 0);

    // texture unit0
    glUniform1i(glGetUniformLocation(splatShader.ID, "u_read_texture"), 0);
    glActiveTexture(GL_TEXTURE0);

    // splat velo
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_force"), 1, glm::value_ptr(mouse_vel));

    for (int z = 0; z < m_depth; ++z)
    {
        glUniform1i(glGetUniformLocation(splatShader.ID, "u_zSlice"), z);

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_velocityTexB, 0, z);
        glBindTexture(GL_TEXTURE_3D, m_velocityTexA); // bind
        glDrawArrays(GL_TRIANGLES, 0, 6); // draw 2d quad
    }
    swapVelocityBuffers();

    // splat dens
    glUniform3f(glGetUniformLocation(splatShader.ID, "u_force"), 1.0f, 0.0f, 0.0f);

    for (int z = 0; z < m_depth; ++z)
    {
        glUniform1i(glGetUniformLocation(splatShader.ID, "u_zSlice"), z);

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_densityTexB, 0, z);
        glBindTexture(GL_TEXTURE_3D, m_densityTexA); // bind
        glDrawArrays(GL_TRIANGLES, 0, 6); // draw 2d quad
    }
    swapDensityBuffers();
    
    // diffuse
    diffuseShader.use();

    // compute constants
    float a = delta_time * viscosity * m_width * m_width; // delicious...
    float rBeta = 1.0f / (1.0f + 6.0f * a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), rBeta);

    // diff velo
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

    // ping-ponging
    for (int i = 0; i < diffuse_iterations; ++i)
    {
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);

        if (i % 2 == 0)
        {
            glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
            for (int z = 0; z < m_depth; ++z)
            {
                glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_zSlice"), z);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_velocityTexB, 0, z);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_3D, m_velocityTexB);
            for (int z = 0; z < m_depth; ++z)
            {
                glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_zSlice"), z);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_velocityTexA, 0, z);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
    
    if (diffuse_iterations % 2 != 0)
    {
        swapVelocityBuffers();
    }

    // diff density
    float density_a = delta_time * 0.00001f * m_width * m_width;
    float density_rBeta = 1.0f / (1.0f + 6.0f * density_a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), density_a);
    glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), density_rBeta);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_densityTexA);
    glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

    for (int i = 0; i < diffuse_iterations; ++i)
    {
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);
        if (i % 2 == 0)
        {
            glBindTexture(GL_TEXTURE_3D, m_densityTexA);
            for (int z = 0; z < m_depth; ++z)
            {
                glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_zSlice"), z);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_densityTexB, 0, z);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_3D, m_densityTexB);
            for (int z = 0; z < m_depth; ++z)
            {
                glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_zSlice"), z);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_densityTexA, 0, z);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }
    if (diffuse_iterations % 2 != 0)
    {
        swapDensityBuffers();
    }

    // advect
    advectShader.use();
    glUniform3f(glGetUniformLocation(advectShader.ID, "u_grid_size"), m_width, m_height, m_depth);
    glUniform1f(glGetUniformLocation(advectShader.ID, "u_dt"), delta_time);

    // adv velo
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_velocity_field"), 0);
    glUniform1i(glGetUniformLocation(advectShader.ID, "u_quantity_to_move"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);

    for (int z = 0; z < m_depth; ++z)
    {
        glUniform1i(glGetUniformLocation(advectShader.ID, "u_zSlice"), z);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_velocityTexB, 0, z);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    swapVelocityBuffers();

    // advect density
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_velocityTexA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_densityTexA);

    for (int z = 0; z < m_depth; ++z)
    {
        glUniform1i(glGetUniformLocation(advectShader.ID, "u_zSlice"), z);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_densityTexB, 0, z);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    swapDensityBuffers();

    // TODO: project, etc.

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}