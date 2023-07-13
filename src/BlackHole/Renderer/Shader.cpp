#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string& filepath)
{
    const std::string shaderFileSrc = Shader::ReadFile(filepath);
    ProcessShaderFile(shaderFileSrc);

    CreateProgram();
}

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    m_ShaderSourceCode[GL_VERTEX_SHADER] = vertexSrc;
    m_ShaderSourceCode[GL_FRAGMENT_SHADER] = fragmentSrc;

    CreateProgram();
}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

void Shader::Bind() const
{
    glUseProgram(m_RendererID);
}

void Shader::Unbind()
{
    glUseProgram(0);
}

void Shader::UploadInt(const std::string& name, int value) const
{
    const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    BH_ASSERT(location != -1, "Uniform doesn't exist!");
    glUniform1i(location, value);
}

void Shader::UploadFloat(const std::string& name, float value) const
{
    const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    BH_ASSERT(location != -1, "Uniform doesn't exist!");
    glUniform1f(location, value);
}

void Shader::UploadFloat3(const std::string& name, const glm::vec3& vector) const
{
    const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    BH_ASSERT(location != -1, "Uniform doesn't exist!");
    glUniform3f(location, vector.x, vector.y, vector.z);
}

void Shader::UploadMat4(const std::string& name, const glm::mat4& matrix) const
{
    const GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    BH_ASSERT(location != -1, "Uniform doesn't exist!");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

std::string Shader::ReadFile(const std::string& filepath)
{
    std::ifstream shaderFile(filepath, std::ios::in | std::ios::binary);
    std::string shaderSrc;

    if (shaderFile.is_open())
	{
		shaderFile.seekg(0, std::ios::end);
		const size_t size = shaderFile.tellg();
		if (size != -1)
		{
			shaderSrc.resize(size);
			shaderFile.seekg(0, std::ios::beg);
			shaderFile.read(shaderSrc.data(), size);
		}
		else
		{
			BH_LOG_ERROR("Could not read from file '{0}'", filepath);
		}
	}
	else
	{
		BH_LOG_ERROR("Could not open file '{0}'", filepath);
	}

    return shaderSrc;
}

GLenum Shader::ShaderTypeFromStringKeyword(const std::string& keyword)
{
    if (keyword == "vertex")
        return GL_VERTEX_SHADER;
    if (keyword == "fragment")
        return GL_FRAGMENT_SHADER;
    if (keyword == "compute")
        return GL_COMPUTE_SHADER;

    BH_ASSERT(false, "Unknown shader type keyword!");
    return 0;
}

void Shader::ProcessShaderFile(const std::string& shaderSources)
{
    const char* typeToken = "#type";
    const size_t tokenSize = strlen(typeToken);

    size_t pos = shaderSources.find(typeToken);
    while (pos != std::string::npos)
    {
        // Find the end of shader type declaration line
        const size_t eol = shaderSources.find_first_of("\r\n", pos);
        BH_ASSERT(eol != std::string::npos, "Syntax error!");

        const size_t beginOfShaderTypeKeyword = pos + tokenSize + 1;
        std::string shaderType = shaderSources.substr(beginOfShaderTypeKeyword, eol - beginOfShaderTypeKeyword);
        BH_ASSERT(Shader::ShaderTypeFromStringKeyword(shaderType), "Invalid shader type specified!");

        const size_t startOfShaderSourceCode = shaderSources.find_first_not_of("\r\n", eol);
        BH_ASSERT(startOfShaderSourceCode != std::string::npos, "Syntax error!");
        pos = shaderSources.find(typeToken, startOfShaderSourceCode);

        m_ShaderSourceCode[Shader::ShaderTypeFromStringKeyword(shaderType)] = (pos == std::string::npos) ?
            shaderSources.substr(startOfShaderSourceCode) :
            shaderSources.substr(startOfShaderSourceCode, pos - startOfShaderSourceCode);
    }
}

void Shader::CreateProgram()
{
    std::unordered_map<GLenum, GLuint> shaderIDs;
    for (const auto& [shaderType, shaderSource] : m_ShaderSourceCode)
    {
        // Create an empty shader handle
        const GLuint shader = glCreateShader(shaderType);

        // Send the shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar* source = shaderSource.c_str();
        glShaderSource(shader, 1, &source, nullptr);

        // Compile the shader
        glCompileShader(shader);

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if(isCompiled == GL_FALSE)
        {
	        GLint maxLength = 0;
	        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

	        // The maxLength includes the NULL character
	        std::vector<GLchar> infoLog(maxLength);
	        glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());
	        
	        // We don't need the shader anymore.
	        glDeleteShader(shader);

	        // Use the infoLog as you see fit.
            BH_LOG_CRITICAL("{0}", infoLog.data());
            BH_ASSERT(false, "Shader compilation failure!");
	        
	        // In this simple program, we'll just leave
	        return;
        }
        shaderIDs[shaderType] = shader;
    }

    // Shaders are successfully compiled.
    // Now time to link them together into a program.
    // Get a program object.
    const GLuint program = glCreateProgram();

    // Attach our shaders to our program
    for (const auto& shaderID : shaderIDs | std::views::values)
        glAttachShader(program, shaderID);

    // Link our program
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
	    GLint maxLength = 0;
	    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

	    // The maxLength includes the NULL character
	    std::vector<GLchar> infoLog(maxLength);
	    glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
	    
	    // We don't need the program anymore.
	    glDeleteProgram(program);

	    // Don't leak shaders either.
	    for (const auto& shaderID : shaderIDs | std::views::values)
            glDeleteShader(shaderID);

	    // Use the infoLog as you see fit.
        BH_LOG_CRITICAL("{0}", infoLog.data());
        BH_ASSERT(false, "Shader link error!");
	    
	    // In this simple program, we'll just leave
	    return;
    }

    // Always detach shaders after a successful link.
    for (const auto& shaderID : shaderIDs | std::views::values)
        glDetachShader(program, shaderID);

    m_RendererID = program;
}