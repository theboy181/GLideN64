#include <Graphics/Context.h>
#include "opengl_ColorBufferReaderWithBufferStorage.h"
#include "opengl_Wrapper.h"

using namespace graphics;
using namespace opengl;

ColorBufferReaderWithBufferStorage::ColorBufferReaderWithBufferStorage(CachedTexture * _pTexture,
	CachedBindBuffer * _bindBuffer)
	: ColorBufferReader(_pTexture), m_bindBuffer(_bindBuffer)
{
	_initBuffers();
}

ColorBufferReaderWithBufferStorage::~ColorBufferReaderWithBufferStorage()
{
	_destroyBuffers();
}

void ColorBufferReaderWithBufferStorage::_initBuffers()
{
	// Generate Pixel Buffer Objects
	FunctionWrapper::glGenBuffers(_numPBO, m_PBO);
	m_curIndex = 0;

	// Initialize Pixel Buffer Objects
	for (int index = 0; index < _numPBO; ++index) {
		m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[index]));
		FunctionWrapper::glBufferStorage(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, std::move(std::unique_ptr<u8[]>(nullptr)), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		m_PBOData[index] = FunctionWrapper::glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pTexture->textureBytes, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	}

	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}

void ColorBufferReaderWithBufferStorage::_destroyBuffers()
{
	auto buffers = std::unique_ptr<GLuint[]>(new GLuint[_numPBO]);

	for(unsigned int index = 0; index < _numPBO; ++index) {
		buffers[index] = m_PBO[index];
	}

	FunctionWrapper::glDeleteBuffers(_numPBO, std::move(buffers));

	for (int index = 0; index < _numPBO; ++index)
		m_PBO[index] = 0;
}

const u8 * ColorBufferReaderWithBufferStorage::_readPixels(const ReadColorBufferParams& _params, u32& _heightOffset,
	u32& _stride)
{
	GLenum format = GLenum(_params.colorFormat);
	GLenum type = GLenum(_params.colorType);

	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[m_curIndex]));
	FunctionWrapper::glReadPixelsAsync(_params.x0, _params.y0, m_pTexture->realWidth, _params.height, format, type);

	if (_params.sync) {
		FunctionWrapper::glFinish();
	}

	_heightOffset = 0;
	_stride = m_pTexture->realWidth;

	return reinterpret_cast<u8*>(m_PBOData[m_curIndex]);
}

void ColorBufferReaderWithBufferStorage::cleanUp()
{
	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}
