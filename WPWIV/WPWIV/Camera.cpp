#include "Camera.h"

Camera::Camera(const XMFLOAT3 &_position,
	const XMFLOAT3 &_target,
	const XMFLOAT3 &_up,
	float _width,
	float _height,
	float _fov,
	float _nearClipPlane,
	float _farClipPlane) :
	position(_position),
	target(_target),
	up(_up),
	width(_width),
	height(_height),
	fov(_fov),
	nearClipPlane(_nearClipPlane),
	farClipPlane(_farClipPlane)
{
}

Camera::~Camera()
{
	ReleaseBuffer();
}

void Camera::ResetCamera(const XMFLOAT3 &_position,
	const XMFLOAT3 &_target,
	const XMFLOAT3 &_up,
	float _width,
	float _height,
	float _fov,
	float _nearClipPlane,
	float _farClipPlane)
{
	position = _position;
	target = _target;
	up = _up;
	width = _width;
	height = _height;
	fov = _fov;
	nearClipPlane = _nearClipPlane;
	farClipPlane = _farClipPlane;
}

bool Camera::InitCamera(ID3D12Device* device)
{
	UpdateViewport();
	UpdateScissorRect();
	UpdateUniform();
	if (!CreateUniformBuffer(device))
		return false;
	UpdateUniformBuffer();
	return true;
}

void Camera::UpdateViewport()
{
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void Camera::UpdateScissorRect()
{
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = width;
	scissorRect.bottom = height;
}

void Camera::UpdateUniform()
{
	XMStoreFloat4x4(
		&uniform.viewProj,
		XMMatrixLookAtLH(
			XMLoadFloat3(&position),
			XMLoadFloat3(&target),
			XMLoadFloat3(&up)) * 
		XMMatrixPerspectiveFovLH(
			XMConvertToRadians(fov), 
			width/height, 
			nearClipPlane, 
			farClipPlane));

	XMStoreFloat4x4(
		&uniform.viewProjInv,
		XMMatrixInverse(
			nullptr, 
			XMLoadFloat4x4(&uniform.viewProj)));
}

bool Camera::CreateUniformBuffer(ID3D12Device* device)
{
	HRESULT hr;

	hr = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraUniform)), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
		D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
		nullptr, // we do not have use an optimized clear value for constant buffers
		IID_PPV_ARGS(&gpuUniformBuffer));
	
	if (FAILED(hr))
	{
		return false;
	}

	gpuUniformBuffer->SetName(L"camera uniform buffer");

	CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

	// map the resource heap to get a gpu virtual address to the beginning of the heap
	hr = gpuUniformBuffer->Map(0, &readRange, &cpuUniformBufferAddress);

	return true;
}

void Camera::UpdateUniformBuffer()
{
	memcpy(cpuUniformBufferAddress, &uniform, sizeof(uniform));
}

void Camera::ReleaseBuffer()
{
	SAFE_RELEASE(gpuUniformBuffer);
}

D3D12_GPU_VIRTUAL_ADDRESS Camera::GetUniformBufferGpuAddress()
{
	return gpuUniformBuffer->GetGPUVirtualAddress();
}

D3D12_VIEWPORT Camera::GetViewport()
{
	return viewport;
}

D3D12_RECT Camera::GetScissorRect()
{
	return scissorRect;
}