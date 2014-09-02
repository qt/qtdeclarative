/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include <GLES2/gl2.h>

extern "C" {

GL_APICALL void GL_APIENTRY glActiveTexture(GLenum texture)
{

}

GL_APICALL void GL_APIENTRY glAttachShader(GLuint program, GLuint shader)
{

}

GL_APICALL void GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar * name)
{

}

GL_APICALL void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{

}

GL_APICALL void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{

}

GL_APICALL void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{

}

GL_APICALL void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{

}

GL_APICALL void GL_APIENTRY glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{

}

GL_APICALL void GL_APIENTRY glBlendEquation(GLenum mode)
{

}

GL_APICALL void GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{

}

GL_APICALL void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{

}

GL_APICALL void GL_APIENTRY glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{

}

GL_APICALL void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage)
{

}

GL_APICALL void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data)
{

}

GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glClear(GLbitfield mask)
{

}

GL_APICALL void GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{

}

GL_APICALL void GL_APIENTRY glClearDepthf(GLfloat d)
{

}

GL_APICALL void GL_APIENTRY glClearStencil(GLint s)
{

}

GL_APICALL void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{

}

GL_APICALL void GL_APIENTRY glCompileShader(GLuint shader)
{

}

GL_APICALL void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void * data)
{

}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void * data)
{

}

GL_APICALL void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{

}

GL_APICALL void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{

}

GL_APICALL GLuint GL_APIENTRY glCreateProgram()
{
    return 0;
}

GL_APICALL GLuint GL_APIENTRY glCreateShader(GLenum type)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glCullFace(GLenum mode)
{

}

GL_APICALL void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint * buffers)
{

}

GL_APICALL void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers)
{

}

GL_APICALL void GL_APIENTRY glDeleteProgram(GLuint program)
{

}

GL_APICALL void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers)
{

}

GL_APICALL void GL_APIENTRY glDeleteShader(GLuint shader)
{

}

GL_APICALL void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint * textures)
{

}

GL_APICALL void GL_APIENTRY glDepthFunc(GLenum func)
{

}

GL_APICALL void GL_APIENTRY glDepthMask(GLboolean flag)
{

}

GL_APICALL void GL_APIENTRY glDepthRangef(GLfloat n, GLfloat f)
{

}

GL_APICALL void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{

}

GL_APICALL void GL_APIENTRY glDisable(GLenum cap)
{

}

GL_APICALL void GL_APIENTRY glDisableVertexAttribArray(GLuint index)
{

}

GL_APICALL void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{

}

GL_APICALL void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const void * indices)
{

}

GL_APICALL void GL_APIENTRY glEnable(GLenum cap)
{

}

GL_APICALL void GL_APIENTRY glEnableVertexAttribArray(GLuint index)
{

}

GL_APICALL void GL_APIENTRY glFinish()
{

}

GL_APICALL void GL_APIENTRY glFlush()
{

}

GL_APICALL void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{

}

GL_APICALL void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{

}

GL_APICALL void GL_APIENTRY glFrontFace(GLenum mode)
{

}

GL_APICALL void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{

}

GL_APICALL void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{

}

GL_APICALL void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{

}

GL_APICALL void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{

}

GL_APICALL void GL_APIENTRY glGenerateMipmap(GLenum target)
{

}

GL_APICALL void GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{

}

GL_APICALL void GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{

}

GL_APICALL void GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders)
{

}

GL_APICALL GLint GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar * name)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* data)
{

}

GL_APICALL void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{

}

GL_APICALL GLenum GL_APIENTRY glGetError()
{
    return 0;
}

GL_APICALL void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* data)
{

}

GL_APICALL void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{

}

GL_APICALL void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* data)
{

}

GL_APICALL void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{

}

GL_APICALL void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{

}

GL_APICALL void GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{

}

GL_APICALL void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{

}

GL_APICALL void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{

}

GL_APICALL void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source)
{

}

GL_APICALL void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{

}

GL_APICALL const GLubyte * GL_APIENTRY glGetString(GLenum name)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{

}

GL_APICALL void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{

}

GL_APICALL GLint GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar * name)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{

}

GL_APICALL void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{

}

GL_APICALL void GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, void ** pointer)
{

}

GL_APICALL void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{

}

GL_APICALL void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{

}

GL_APICALL void GL_APIENTRY glHint(GLenum target, GLenum mode)
{

}

GL_APICALL GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsFramebuffer(GLuint framebuffer)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsProgram(GLuint program)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
    return 0;
}

GL_APICALL GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
    return 0;
}

GL_APICALL void GL_APIENTRY glLineWidth(GLfloat width)
{

}

GL_APICALL void GL_APIENTRY glLinkProgram(GLuint program)
{

}

GL_APICALL void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{

}

GL_APICALL void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{

}

GL_APICALL void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void * pixels)
{

}

GL_APICALL void GL_APIENTRY glReleaseShaderCompiler()
{

}

GL_APICALL void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{

}

GL_APICALL void GL_APIENTRY glSampleCoverage(GLfloat value, GLboolean invert)
{

}

GL_APICALL void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{

}

GL_APICALL void GL_APIENTRY glShaderBinary(GLsizei count, const GLuint * shaders, GLenum binaryformat, const void * binary, GLsizei length)
{

}

GL_APICALL void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length)
{

}

GL_APICALL void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{

}

GL_APICALL void GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{

}

GL_APICALL void GL_APIENTRY glStencilMask(GLuint mask)
{

}

GL_APICALL void GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{

}

GL_APICALL void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{

}

GL_APICALL void GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
{

}

GL_APICALL void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels)
{

}

GL_APICALL void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{

}

GL_APICALL void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{

}

GL_APICALL void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{

}

GL_APICALL void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint * params)
{

}

GL_APICALL void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels)
{

}

GL_APICALL void GL_APIENTRY glUniform1f(GLint location, GLfloat v0)
{

}

GL_APICALL void GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniform1i(GLint location, GLint v0)
{

}

GL_APICALL void GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint * value)
{

}

GL_APICALL void GL_APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1)
{

}

GL_APICALL void GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniform2i(GLint location, GLint v0, GLint v1)
{

}

GL_APICALL void GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint * value)
{

}

GL_APICALL void GL_APIENTRY glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{

}

GL_APICALL void GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniform3i(GLint location, GLint v0, GLint v1, GLint v2)
{

}

GL_APICALL void GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint * value)
{

}

GL_APICALL void GL_APIENTRY glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{

}

GL_APICALL void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{

}

GL_APICALL void GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint * value)
{

}

GL_APICALL void GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{

}

GL_APICALL void GL_APIENTRY glUseProgram(GLuint program)
{

}

GL_APICALL void GL_APIENTRY glValidateProgram(GLuint program)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib1f(GLuint index, GLfloat x)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat * v)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat * v)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat * v)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{

}

GL_APICALL void GL_APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat * v)
{

}

GL_APICALL void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer)
{

}

GL_APICALL void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{

}

GL_APICALL void GL_APIENTRY glBlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{

}

GL_APICALL void GL_APIENTRY glRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{

}

} // extern "C"

