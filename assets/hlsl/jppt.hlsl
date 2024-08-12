#pragma pack_matrix( row_major )

struct Ray
{
  float3 Origin;
  float3 Direction;
  float3 InverseDirection;
};

struct Triangle
{
  float3 v0;
  float  padding0;
  float3 v1;
  float  padding1;
  float3 v2;
  float  padding2;
  float3 Centroid;
  float  padding3;
};

struct TriangleExtraData
{
  float3 Normal0;
  float  padding0;
  float3 Normal1;
  float  padding1;
  float3 Normal2;
  float  padding2;
    
  float2 UV0, UV1, UV2;
  float2 padding3;

  float4 Colour0;
  float4 Colour1;
  float4 Colour2;

  float4 Tangent0;
  float4 Tangent1;
  float4 Tangent2;
};

struct BVHNode
{
  float3 AABBMin;
  float  padding0;
  float3 AABBMax;
  float  padding1;
  uint   LeftChildOrFirst;
  uint   TriangleCount;
  float2 padding2;
};

struct IndexData
{
  uint TriangleDataStartInx;
  uint IndicesDataStartInx;
  uint BVHNodeDataStartInx;
  uint TriangleCount;
};

struct AABB
{
  float3 Min;
  float  pad0;
  float3 Max;
  float  pad1;
};

struct BVHInstance
{
  matrix InverseTransform;
  matrix Transform;
  matrix NormalTransform;
  AABB   Bounds;

  uint   MeshIndex;
  uint   Index;
  float2 Pad;
};

struct TLASNode
{
  float3 AABBMin;
  uint   LeftRight;
  float3 AABBMax;
  uint   BLAS;
};

struct Camera
{
  matrix Frame;
      
  float  Lens;
  float  Film;
  float  Aspect;
  float  Focus;
    
  float3 Padding0;
  float  Aperture;
    
  int    Orthographic;
  float3 Padding;
};



layout (binding = 0, rgba32f)
uniform image2D RenderImage;


layout(std430, binding = 8)
buffer CameraBuffer
{
  CameraCameras[];
};

// BVH
layout(std430, binding = 1)
buffer TriangleBuffer
{
    Triangle
  TriangleBuffer[];
};

layout(std430, binding = 2)
buffer TriangleExBuffer
{
  TriangleExtraDataTriangleExBuffer[];
};

layout(std430, binding = 3)
buffer bvhBuffer
{
  BVHNodeBVHBuffer[];
};

layout(std430, binding = 4)
buffer indicesBuffer
{

uint IndicesBuffer[];
};

layout(std430, binding = 5)
buffer IndexDataBuffer
{
  IndexDataIndexDataBuffer[];
};

layout(std430, binding = 6)
buffer tlasInstancesBuffer
{
  BVHInstanceTLASInstancesBuffer[];
};

layout(std430, binding = 7)
buffer TLASNodes
{
  TLASNodeTLASNodes[];
};

struct SceneIntersection
{
  float Distance;
  uint  InstanceIndex;
  uint  PrimitiveIndex;
  float U;
  float V;
};

float RayAABBIntersection(Ray ray, float3 AABBMin, float3 AABBMax, inout SceneIntersection Isect)
{
  float tx1 = (AABBMin.x - ray.Origin.x) * ray.InverseDirection.x;
  float tx2 = (AABBMax.x - ray.Origin.x) * ray.InverseDirection.x;
  float tmin = min(tx1, tx2), tmax = max(tx1, tx2);

  float ty1 = (AABBMin.y - ray.Origin.y) * ray.InverseDirection.y;
  float ty2 = (AABBMax.y - ray.Origin.y) * ray.InverseDirection.y;
  tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));

  float tz1 = (AABBMin.z - ray.Origin.z) * ray.InverseDirection.z;
  float tz2 = (AABBMax.z - ray.Origin.z) * ray.InverseDirection.z;
  tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));

  if (tmax >= tmin && tmin < Isect.Distance && tmax > 0)
    return tmin;
  else
    return 1e30f;
}

void RayTriangleInteresection(Ray ray,
                              Triangle Triangle,
                              inout SceneIntersection Isect,
                              uint InstanceIndex,
                              uint PrimitiveIndex)
{
  float3 Edge1 = Triangle.v1 - Triangle.v0;
  float3 Edge2 = Triangle.v2 - Triangle.v0;

  float3 h = cross(ray.Direction, Edge2);
  float a = dot(Edge1, h);
  if (a > -0.000001f && a < 0.000001f)
    return; //Ray is parallel to the Triangle
    
  float f = 1 / a;
  float3 s = ray.Origin - Triangle.v0;
  float u = f * dot(s, h);
  if (u < 0 || u > 1)
    return;

  float3 q = cross(s, Edge1);
  float v = f * dot(ray.Direction, q);
  if (v < 0 || u + v > 1)
    return;
    
  float t = f * dot(Edge2, q);
  if (t > 0.000001f && t < Isect.Distance)
  {
    Isect.InstanceIndex = InstanceIndex;
    Isect.PrimitiveIndex = PrimitiveIndex;
    Isect.U = u;
    Isect.V = v;
    Isect.Distance = t;
  }
}

void IntersectBVH(Ray ray,
                  inout SceneIntersection Isect,
                  uint InstanceIndex,
                  uint MeshIndex)
{
  uint NodeInx = 0;
  uint Stack[64];
  uint StackPointer = 0;
  bool t = true;
    
  IndexData IndexData = IndexDataBuffer[MeshIndex];
  uint NodeStartInx = IndexData.BVHNodeDataStartInx;
  uint TriangleStartInx = IndexData.TriangleDataStartInx;
  uint IndexStartInx = IndexData.IndicesDataStartInx;
    
  //We start with the root node of the shape 
  while (t)
  {

    // The current node contains Triangles, it's a leaf. 
    if (BVHBuffer[NodeStartInx + NodeInx].TriangleCount > 0)
    {
            // For each Triangle in the leaf, intersect them
      for ( uint i = 0; i < BVHBuffer[NodeStartInx + NodeInx].  TriangleCount; i++)
      {
        uint Index = TriangleStartInx + IndicesBuffer[IndexStartInx + BVHBuffer[NodeStartInx + NodeInx].LeftChildOrFirst + i];
        RayTriangleInteresection(ray,
                                 TriangleBuffer[Index],
                                 Isect,
                                 InstanceIndex,
                                 Index);
      }
            // Go back up the stack and continue to process the next node on the stack
      if (StackPointer == 0)
        break;
      else
        NodeInx = Stack[--StackPointer];
      continue;
    }

        // Get the 2 children of the current node
    uint Child1 = BVHBuffer[NodeStartInx + NodeInx].LeftChildOrFirst;
    uint Child2 = BVHBuffer[NodeStartInx + NodeInx].LeftChildOrFirst + 1;

        // Intersect with the 2 AABB boxes, and get the closest hit
    float Dist1 = RayAABBIntersection(ray, BVHBuffer[Child1 + NodeStartInx].AABBMin, BVHBuffer[Child1 + NodeStartInx].AABBMax, Isect);
    float Dist2 = RayAABBIntersection(ray, BVHBuffer[Child2 + NodeStartInx].AABBMin, BVHBuffer[Child2 + NodeStartInx].AABBMax, Isect);
    if (Dist1 > Dist2)
    {
      float tmpDist = Dist2;
      Dist2 = Dist1;
      Dist1 = tmpDist;

      uint tmpChild = Child2;
      Child2 = Child1;
      Child1 = tmpChild;
    }

    if (Dist1 == 1e30f)
    {
            // If we didn't hit any of the 2 child, we can go up the stack
      if (StackPointer == 0)
        break;
      else
        NodeInx = Stack[--StackPointer];
    }
    else
    {
      // If we did hit, add this child to the stack.
      NodeInx = Child1;
      if (Dist2 != 1e30f)
      {
        Stack[StackPointer++] = Child2;
      }
    }
  }
}

void IntersectInstance(Ray ray,
                       inout SceneIntersection Isect,
                       uint InstanceIndex)
{
  matrix InverseTransform = TLASInstancesBuffer[InstanceIndex].InverseTransform;
  ray.Origin = float3((InverseTransform * float4(ray.Origin, 1)));
  ray.Direction = float3((InverseTransform * float4(ray.Direction, 0)));
  ray.InverseDirection = 1.0f / ray.  Direction;

  IntersectBVH(ray, Isect, TLASInstancesBuffer[InstanceIndex].Index, TLASInstancesBuffer[InstanceIndex].MeshIndex);
}

void IntersectTLAS(Ray ray, inout SceneIntersection Isect)
{
  ray.InverseDirection = 1.0f / ray.  Direction;
  uint NodeInx = 0;
  uint Stack[64];
  uint StackPtr = 0;
  while (true)
  {
    //If we hit the leaf, check intersection with the bvhs
    if (TLASNodes[NodeInx].LeftRight == 0)
    {
      IntersectInstance(ray, Isect, TLASNodes[NodeInx].BLAS);
            
      if (StackPtr == 0)
        break;
      else
        NodeInx = Stack[--StackPtr];
      continue;
    }

    //Check if hit any of the children
    uint Child1 = TLASNodes[NodeInx].LeftRight & 0xffff;
    uint Child2 = TLASNodes[NodeInx].LeftRight >> 16;
        
    float Dist1 = RayAABBIntersection(ray, TLASNodes[Child1].AABBMin, TLASNodes[Child1].AABBMax, Isect);
    float Dist2 = RayAABBIntersection(ray, TLASNodes[Child2].AABBMin, TLASNodes[Child2].AABBMax, Isect);
    if (Dist1 > Dist2)
    { //Swap if dist 2 is closer
      float tmpDist = Dist2;
      Dist2 = Dist1;
      Dist1 = tmpDist;

      uint tmpChild = Child2;
      Child2 = Child1;
      Child1 = tmpChild;
    }
        
    if (Dist1 == 1e30f) //We didn't hit a child
    {
      if (StackPtr == 0)
        break; //There's no node left to explore
      else
        NodeInx = Stack[--StackPtr]; //Go to the next node in the stack
    }
    else //We hit a child
    {
      NodeInx = Child1; //Set the current node to the first child
      if (Dist2 != 1e30f)
        Stack[StackPtr++] = Child2; //If we also hit the other node, add it in the stack
    }
  }
}


Ray MakeRay( float3 Origin, float3 Direction, float3 InverseDirection)
{
    Ray ray;
    ray.Origin = Origin;
    ray.Direction = Direction;
    ray.InverseDirection = InverseDirection;
    return ray;
}

float3 TransformPoint( matrix A, float3 B)

{
  float4 Res = mul(A, float4(B, 1));
  return Res.xyz / Res.w;
}

float3 TransformDirection( matrix A, float3 B)
{
  return normalize(mul(A, float4(B, 0))).xyz;
}

Ray GetRay( float2 ImageUV)
{
  Camera camera = cameras[0];

    // Point on the film
  float3 Q = float3(
        (0.5f - ImageUV.x),
        (ImageUV.y - 0.5f),
        1
    );
  float3 RayDirection = -normalize(Q);
  float3 PointOnLens = float3(0, 0, 0);

    //Transform the Ray direction and origin
  Ray ray = MakeRay(
        TransformPoint(camera.Frame, PointOnLens),
        TransformDirection(camera.Frame, RayDirection),
        float3(0)
    );
  return ray;
}









RWTexture2D<float4> sOutputTexture : register( u0 );

[numthreads(8,8,1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{
  uint width, height;
  sOutputTexture.GetDimensions(width, height);
  if (id.x >= width || id.y >= height)
    return;

  float u = (float) id.x / width;
  float v = 1 - (float) id.y / height;
  float2 uv = float2(u, v);
  float2 color_sRGB = float2(u, v);
  float2 color_linear = pow(color_sRGB, 2.2);
  // our texture is linear (rgba8unorm cannot be used with typed uav),
  // so when it is sampled, it will be... linear(?).
  // 
  // and our backbuffer is linear (rgba16f), so the pixel shader should return linear color
  // and this shader should return linear color which goes into the texture
  sOutputTexture[id.xy] = float4( color_linear, 0, 1);

  
  // -----------------------------------------------------------------------------------------------

  Ray ray = GetRay(uv);

  SceneIntersection Isect;
  Isect.Distance = 1e30f;

  IntersectTLAS(ray, Isect);
  if(Isect.Distance < 1e30f)
  {
    TriangleExtraData ExtraData = TriangleExBuffer[Isect.PrimitiveIndex];
    float4 Colour = ExtraData.Colour1 * Isect.U +
                    ExtraData.Colour2 * Isect.V +
                    ExtraData.Colour0 * (1 - Isect.U - Isect.V);
    sOutputTexture[id.xy] = Colour;
  }
}
