#include "frame/opengl/message_callback.h"

#include <string>

#include "frame/logger.h"

namespace frame::opengl {

    // Private space.
    namespace {

        std::string Source2String(GLenum source) {
            switch (source) {
            case GL_DEBUG_SOURCE_API:
                return "Calls to the OpenGL API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "Calls to a window - system API";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "A compiler for a shading language";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "An application associated with OpenGL";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "Generated by the user of this application";
            case GL_DEBUG_SOURCE_OTHER:
                return "Some source that isn't one of these";
            default:
                return "???";
            }
        }

        std::string Type2String(GLenum type) {
            switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "An error, typically from the API";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "Some behavior marked deprecated has been used";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "Something has invoked undefined behavior";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "Some functionality the user relies upon is not"
                    " portable";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "Code has triggered possible performance issues";
            case GL_DEBUG_TYPE_MARKER:
                return "Command stream annotation";
            case GL_DEBUG_TYPE_PUSH_GROUP:
                return "Group pushing";
            case GL_DEBUG_TYPE_POP_GROUP:
                return "Group popping";
            case GL_DEBUG_TYPE_OTHER:
                return "Some type that isn't one of these";
            default:
                return "???";
            }
        }

        std::string Severity2String(GLenum severity) {
            switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                return "All OpenGL Errors, shader compilation / linking "
                    "errors, or highly - dangerous undefined behavior";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "Major performance warnings, shader compilation / "
                    "linking warnings, or the use of deprecated functionality";
            case GL_DEBUG_SEVERITY_LOW:
                return "Redundant state change performance warning, or "
                    "unimportant undefined behavior";
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "Anything that isn't an error or performance issue";
            default:
                return "???";
            }
        }

    }  // End namespace.

    void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam) {
        Logger& logger = Logger::GetInstance();
        std::string str_message =
            fmt::format(
                "GL: {} (source [{}], type [{}], severity [{}])",
                message,
                Source2String(source),
                Type2String(type),
                Severity2String(severity));
        if ((type == GL_DEBUG_TYPE_ERROR) ||
            (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR))
        {
            logger->error(str_message);
        }
        else if (
            (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) ||
            (type == GL_DEBUG_TYPE_PERFORMANCE) ||
            (type == GL_DEBUG_TYPE_PORTABILITY))
        {
            logger->warn(str_message);
        }
        else {
            logger->info(str_message);
        }
    }

}  // End namespace frame::opengl.
