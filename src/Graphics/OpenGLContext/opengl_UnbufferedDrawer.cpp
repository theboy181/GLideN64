#include <Config.h>
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include "opengl_CachedFunctions.h"
#include "opengl_UnbufferedDrawer.h"
#include "opengl_Wrapper.h"

#include <algorithm>

using namespace opengl;

UnbufferedDrawer::UnbufferedDrawer(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray)
		: m_glInfo(_glinfo)
		, m_cachedAttribArray(_cachedAttribArray)
{
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	m_attribsData.fill(nullptr);
}

UnbufferedDrawer::~UnbufferedDrawer()
{
}

bool UnbufferedDrawer::_updateAttribPointer(u32 _index, const void * _ptr)
{
	if (m_attribsData[_index] == _ptr)
		return false;
	m_attribsData[_index] = _ptr;
	return true;
}

void UnbufferedDrawer::drawTriangles(const graphics::Context::DrawTriangleParameters & _params)
{
	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
		const void * ptr = &_params.vertices->x;
		if (_updateAttribPointer(triangleAttrib::position, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	if (_params.combiner->usesShade()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		const void * ptr = _params.flatColors ? &_params.vertices->flat_r : &_params.vertices->r;
		if (_updateAttribPointer(triangleAttrib::color, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}
	else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);

	if (_params.combiner->usesTexture()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, true);
		const void * ptr = &_params.vertices->s;
		if (_updateAttribPointer(triangleAttrib::texcoord, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, true);
		const void * ptr = &_params.vertices->modify;
		if (_updateAttribPointer(triangleAttrib::modify, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::modify, 4, GL_BYTE, GL_FALSE, sizeof(SPVertex), ptr);
	}

	if (config.generalEmulation.enableHWLighting != 0)
		FunctionWrapper::glVertexAttrib1f(triangleAttrib::numlights, GLfloat(_params.vertices[0].HWLight));

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	if (_params.elements == nullptr) {
		FunctionWrapper::glDrawArrays(GLenum(_params.mode), 0, _params.verticesCount);
		return;
	}

	if (config.frameBufferEmulation.N64DepthCompare == 0) {
		FunctionWrapper::glDrawElementsNotThreadSafe(GLenum(_params.mode), _params.elementsCount, GL_UNSIGNED_BYTE, _params.elements);
		return;
	}

	// Draw polygons one by one
	for (GLint i = 0; i < _params.elementsCount; i += 3) {
		FunctionWrapper::glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		FunctionWrapper::glDrawElementsNotThreadSafe(GLenum(_params.mode), 3, GL_UNSIGNED_BYTE, (u8*)_params.elements + i);
	}
}

void UnbufferedDrawer::drawRects(const graphics::Context::DrawRectParameters & _params)
{
	{
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, true);
		const void * ptr = &_params.vertices->x;
		if (_updateAttribPointer(rectAttrib::position, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(rectAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	}

	if (_params.texrect && _params.combiner->usesTile(0)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, true);
		const void * ptr = &_params.vertices->s0;
		if (_updateAttribPointer(rectAttrib::texcoord0, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(rectAttrib::texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);

	if (_params.texrect && _params.combiner->usesTile(1)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, true);
		const void * ptr = &_params.vertices->s1;
		if (_updateAttribPointer(rectAttrib::texcoord1, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(rectAttrib::texcoord1, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	FunctionWrapper::glDrawArrays(GLenum(_params.mode), 0, _params.verticesCount);
}

void UnbufferedDrawer::drawLine(f32 _width, SPVertex * _vertices)
{
	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
		const void * ptr = &_vertices->x;
		if (_updateAttribPointer(triangleAttrib::position, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		const void * ptr = &_vertices->r;
		if (_updateAttribPointer(triangleAttrib::color, ptr))
			FunctionWrapper::glVertexAttribPointerNotThreadSafe(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	FunctionWrapper::glLineWidth(_width);
	FunctionWrapper::glDrawArrays(GL_LINES, 0, 2);
}