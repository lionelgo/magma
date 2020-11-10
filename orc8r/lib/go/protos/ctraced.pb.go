// Code generated by protoc-gen-go. DO NOT EDIT.
// source: orc8r/protos/ctraced.proto

package protos

import (
	context "context"
	fmt "fmt"
	proto "github.com/golang/protobuf/proto"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
	math "math"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion3 // please upgrade the proto package

type StartTraceRequest_TraceType int32

const (
	StartTraceRequest_ALL        StartTraceRequest_TraceType = 0
	StartTraceRequest_SUBSCRIBER StartTraceRequest_TraceType = 1
	StartTraceRequest_PROTOCOL   StartTraceRequest_TraceType = 2
	StartTraceRequest_INTERFACE  StartTraceRequest_TraceType = 3
)

var StartTraceRequest_TraceType_name = map[int32]string{
	0: "ALL",
	1: "SUBSCRIBER",
	2: "PROTOCOL",
	3: "INTERFACE",
}

var StartTraceRequest_TraceType_value = map[string]int32{
	"ALL":        0,
	"SUBSCRIBER": 1,
	"PROTOCOL":   2,
	"INTERFACE":  3,
}

func (x StartTraceRequest_TraceType) String() string {
	return proto.EnumName(StartTraceRequest_TraceType_name, int32(x))
}

func (StartTraceRequest_TraceType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{0, 0}
}

type StartTraceRequest_ProtocolName int32

const (
	StartTraceRequest_SCTP     StartTraceRequest_ProtocolName = 0
	StartTraceRequest_DIAMETER StartTraceRequest_ProtocolName = 1
)

var StartTraceRequest_ProtocolName_name = map[int32]string{
	0: "SCTP",
	1: "DIAMETER",
}

var StartTraceRequest_ProtocolName_value = map[string]int32{
	"SCTP":     0,
	"DIAMETER": 1,
}

func (x StartTraceRequest_ProtocolName) String() string {
	return proto.EnumName(StartTraceRequest_ProtocolName_name, int32(x))
}

func (StartTraceRequest_ProtocolName) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{0, 1}
}

type StartTraceRequest_InterfaceName int32

const (
	StartTraceRequest_S1AP StartTraceRequest_InterfaceName = 0
	StartTraceRequest_GX   StartTraceRequest_InterfaceName = 1
	StartTraceRequest_GT   StartTraceRequest_InterfaceName = 2
)

var StartTraceRequest_InterfaceName_name = map[int32]string{
	0: "S1AP",
	1: "GX",
	2: "GT",
}

var StartTraceRequest_InterfaceName_value = map[string]int32{
	"S1AP": 0,
	"GX":   1,
	"GT":   2,
}

func (x StartTraceRequest_InterfaceName) String() string {
	return proto.EnumName(StartTraceRequest_InterfaceName_name, int32(x))
}

func (StartTraceRequest_InterfaceName) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{0, 2}
}

type StartTraceRequest struct {
	TraceType StartTraceRequest_TraceType `protobuf:"varint,1,opt,name=trace_type,json=traceType,proto3,enum=magma.orc8r.StartTraceRequest_TraceType" json:"trace_type,omitempty"`
	// IMSI specified only if trace_type is SUBSCRIBER
	Imsi string `protobuf:"bytes,2,opt,name=imsi,proto3" json:"imsi,omitempty"`
	// Protocol name specified only if trace_type is PROTOCOL
	Protocol StartTraceRequest_ProtocolName `protobuf:"varint,3,opt,name=protocol,proto3,enum=magma.orc8r.StartTraceRequest_ProtocolName" json:"protocol,omitempty"`
	// Interface name specified only if trace_type is INTERFACE
	Interface            StartTraceRequest_InterfaceName `protobuf:"varint,4,opt,name=interface,proto3,enum=magma.orc8r.StartTraceRequest_InterfaceName" json:"interface,omitempty"`
	XXX_NoUnkeyedLiteral struct{}                        `json:"-"`
	XXX_unrecognized     []byte                          `json:"-"`
	XXX_sizecache        int32                           `json:"-"`
}

func (m *StartTraceRequest) Reset()         { *m = StartTraceRequest{} }
func (m *StartTraceRequest) String() string { return proto.CompactTextString(m) }
func (*StartTraceRequest) ProtoMessage()    {}
func (*StartTraceRequest) Descriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{0}
}

func (m *StartTraceRequest) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_StartTraceRequest.Unmarshal(m, b)
}
func (m *StartTraceRequest) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_StartTraceRequest.Marshal(b, m, deterministic)
}
func (m *StartTraceRequest) XXX_Merge(src proto.Message) {
	xxx_messageInfo_StartTraceRequest.Merge(m, src)
}
func (m *StartTraceRequest) XXX_Size() int {
	return xxx_messageInfo_StartTraceRequest.Size(m)
}
func (m *StartTraceRequest) XXX_DiscardUnknown() {
	xxx_messageInfo_StartTraceRequest.DiscardUnknown(m)
}

var xxx_messageInfo_StartTraceRequest proto.InternalMessageInfo

func (m *StartTraceRequest) GetTraceType() StartTraceRequest_TraceType {
	if m != nil {
		return m.TraceType
	}
	return StartTraceRequest_ALL
}

func (m *StartTraceRequest) GetImsi() string {
	if m != nil {
		return m.Imsi
	}
	return ""
}

func (m *StartTraceRequest) GetProtocol() StartTraceRequest_ProtocolName {
	if m != nil {
		return m.Protocol
	}
	return StartTraceRequest_SCTP
}

func (m *StartTraceRequest) GetInterface() StartTraceRequest_InterfaceName {
	if m != nil {
		return m.Interface
	}
	return StartTraceRequest_S1AP
}

type StartTraceResponse struct {
	Success              bool     `protobuf:"varint,1,opt,name=success,proto3" json:"success,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *StartTraceResponse) Reset()         { *m = StartTraceResponse{} }
func (m *StartTraceResponse) String() string { return proto.CompactTextString(m) }
func (*StartTraceResponse) ProtoMessage()    {}
func (*StartTraceResponse) Descriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{1}
}

func (m *StartTraceResponse) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_StartTraceResponse.Unmarshal(m, b)
}
func (m *StartTraceResponse) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_StartTraceResponse.Marshal(b, m, deterministic)
}
func (m *StartTraceResponse) XXX_Merge(src proto.Message) {
	xxx_messageInfo_StartTraceResponse.Merge(m, src)
}
func (m *StartTraceResponse) XXX_Size() int {
	return xxx_messageInfo_StartTraceResponse.Size(m)
}
func (m *StartTraceResponse) XXX_DiscardUnknown() {
	xxx_messageInfo_StartTraceResponse.DiscardUnknown(m)
}

var xxx_messageInfo_StartTraceResponse proto.InternalMessageInfo

func (m *StartTraceResponse) GetSuccess() bool {
	if m != nil {
		return m.Success
	}
	return false
}

type EndTraceRequest struct {
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *EndTraceRequest) Reset()         { *m = EndTraceRequest{} }
func (m *EndTraceRequest) String() string { return proto.CompactTextString(m) }
func (*EndTraceRequest) ProtoMessage()    {}
func (*EndTraceRequest) Descriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{2}
}

func (m *EndTraceRequest) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_EndTraceRequest.Unmarshal(m, b)
}
func (m *EndTraceRequest) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_EndTraceRequest.Marshal(b, m, deterministic)
}
func (m *EndTraceRequest) XXX_Merge(src proto.Message) {
	xxx_messageInfo_EndTraceRequest.Merge(m, src)
}
func (m *EndTraceRequest) XXX_Size() int {
	return xxx_messageInfo_EndTraceRequest.Size(m)
}
func (m *EndTraceRequest) XXX_DiscardUnknown() {
	xxx_messageInfo_EndTraceRequest.DiscardUnknown(m)
}

var xxx_messageInfo_EndTraceRequest proto.InternalMessageInfo

type EndTraceResponse struct {
	Success              bool     `protobuf:"varint,1,opt,name=success,proto3" json:"success,omitempty"`
	TraceContent         []byte   `protobuf:"bytes,2,opt,name=trace_content,json=traceContent,proto3" json:"trace_content,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *EndTraceResponse) Reset()         { *m = EndTraceResponse{} }
func (m *EndTraceResponse) String() string { return proto.CompactTextString(m) }
func (*EndTraceResponse) ProtoMessage()    {}
func (*EndTraceResponse) Descriptor() ([]byte, []int) {
	return fileDescriptor_74b70534723ed60d, []int{3}
}

func (m *EndTraceResponse) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_EndTraceResponse.Unmarshal(m, b)
}
func (m *EndTraceResponse) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_EndTraceResponse.Marshal(b, m, deterministic)
}
func (m *EndTraceResponse) XXX_Merge(src proto.Message) {
	xxx_messageInfo_EndTraceResponse.Merge(m, src)
}
func (m *EndTraceResponse) XXX_Size() int {
	return xxx_messageInfo_EndTraceResponse.Size(m)
}
func (m *EndTraceResponse) XXX_DiscardUnknown() {
	xxx_messageInfo_EndTraceResponse.DiscardUnknown(m)
}

var xxx_messageInfo_EndTraceResponse proto.InternalMessageInfo

func (m *EndTraceResponse) GetSuccess() bool {
	if m != nil {
		return m.Success
	}
	return false
}

func (m *EndTraceResponse) GetTraceContent() []byte {
	if m != nil {
		return m.TraceContent
	}
	return nil
}

func init() {
	proto.RegisterEnum("magma.orc8r.StartTraceRequest_TraceType", StartTraceRequest_TraceType_name, StartTraceRequest_TraceType_value)
	proto.RegisterEnum("magma.orc8r.StartTraceRequest_ProtocolName", StartTraceRequest_ProtocolName_name, StartTraceRequest_ProtocolName_value)
	proto.RegisterEnum("magma.orc8r.StartTraceRequest_InterfaceName", StartTraceRequest_InterfaceName_name, StartTraceRequest_InterfaceName_value)
	proto.RegisterType((*StartTraceRequest)(nil), "magma.orc8r.StartTraceRequest")
	proto.RegisterType((*StartTraceResponse)(nil), "magma.orc8r.StartTraceResponse")
	proto.RegisterType((*EndTraceRequest)(nil), "magma.orc8r.EndTraceRequest")
	proto.RegisterType((*EndTraceResponse)(nil), "magma.orc8r.EndTraceResponse")
}

func init() { proto.RegisterFile("orc8r/protos/ctraced.proto", fileDescriptor_74b70534723ed60d) }

var fileDescriptor_74b70534723ed60d = []byte{
	// 428 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x84, 0x93, 0xcf, 0x6e, 0xd3, 0x40,
	0x10, 0xc6, 0xe3, 0x24, 0x6a, 0xe3, 0xc1, 0x09, 0xdb, 0x39, 0x99, 0xf0, 0xaf, 0x5a, 0x24, 0x14,
	0x04, 0x72, 0x44, 0xb9, 0x70, 0x75, 0x8c, 0x89, 0x82, 0xd2, 0x26, 0xac, 0x8d, 0x84, 0xb8, 0x20,
	0x77, 0xb3, 0x54, 0x96, 0x12, 0x3b, 0x78, 0xb7, 0x48, 0x7d, 0x16, 0xde, 0x83, 0xe7, 0x43, 0x19,
	0xd7, 0x89, 0x0b, 0x4a, 0x73, 0xda, 0xdd, 0x99, 0xef, 0xfb, 0xe9, 0x1b, 0x8d, 0x16, 0xfa, 0x79,
	0x21, 0xdf, 0x17, 0xc3, 0x75, 0x91, 0x9b, 0x5c, 0x0f, 0xa5, 0x29, 0x12, 0xa9, 0x16, 0x1e, 0x3d,
	0xf1, 0xc1, 0x2a, 0xb9, 0x5a, 0x25, 0x1e, 0x29, 0xf8, 0xef, 0x16, 0x9c, 0x44, 0x26, 0x29, 0x4c,
	0xbc, 0x91, 0x08, 0xf5, 0xf3, 0x5a, 0x69, 0x83, 0x63, 0x00, 0xb2, 0x7c, 0x37, 0x37, 0x6b, 0xe5,
	0x5a, 0xa7, 0xd6, 0xa0, 0x77, 0x36, 0xf0, 0x6a, 0x3e, 0xef, 0x3f, 0x8f, 0x47, 0x8f, 0xf8, 0x66,
	0xad, 0x84, 0x6d, 0xaa, 0x2b, 0x22, 0xb4, 0xd3, 0x95, 0x4e, 0xdd, 0xe6, 0xa9, 0x35, 0xb0, 0x05,
	0xdd, 0x71, 0x0c, 0x1d, 0x0a, 0x22, 0xf3, 0xa5, 0xdb, 0x22, 0xf4, 0xeb, 0x03, 0xe8, 0xf9, 0xad,
	0xfc, 0x22, 0x59, 0x29, 0xb1, 0x35, 0xe3, 0x27, 0xb0, 0xd3, 0xcc, 0xa8, 0xe2, 0x47, 0x22, 0x95,
	0xdb, 0x26, 0xd2, 0x9b, 0x03, 0xa4, 0x49, 0xa5, 0x27, 0xd4, 0xce, 0xce, 0x7d, 0xb0, 0xb7, 0x03,
	0xe0, 0x31, 0xb4, 0xfc, 0xe9, 0x94, 0x35, 0xb0, 0x07, 0x10, 0x7d, 0x19, 0x45, 0x81, 0x98, 0x8c,
	0x42, 0xc1, 0x2c, 0x74, 0xa0, 0x33, 0x17, 0xb3, 0x78, 0x16, 0xcc, 0xa6, 0xac, 0x89, 0x5d, 0xb0,
	0x27, 0x17, 0x71, 0x28, 0x3e, 0xfa, 0x41, 0xc8, 0x5a, 0xfc, 0x25, 0x38, 0xf5, 0xa0, 0xd8, 0x81,
	0x76, 0x14, 0xc4, 0x73, 0xd6, 0xd8, 0xd8, 0x3e, 0x4c, 0xfc, 0xf3, 0x30, 0xde, 0x40, 0xf8, 0x2b,
	0xe8, 0xde, 0x89, 0x41, 0xc2, 0xb7, 0xfe, 0x46, 0x78, 0x04, 0xcd, 0xf1, 0x57, 0x66, 0xd1, 0x19,
	0xb3, 0x26, 0xf7, 0x00, 0xeb, 0x33, 0xe8, 0x75, 0x9e, 0x69, 0x85, 0x2e, 0x1c, 0xeb, 0x6b, 0x29,
	0x95, 0xd6, 0xb4, 0x9a, 0x8e, 0xa8, 0x9e, 0xfc, 0x04, 0x1e, 0x86, 0xd9, 0xa2, 0x3e, 0x31, 0xff,
	0x0c, 0x6c, 0x57, 0x3a, 0x04, 0xc0, 0x17, 0xd0, 0x2d, 0x17, 0x2f, 0xf3, 0xcc, 0xa8, 0xcc, 0xd0,
	0xe2, 0x1c, 0xe1, 0x50, 0x31, 0x28, 0x6b, 0x67, 0x7f, 0x2c, 0x60, 0x41, 0xb2, 0x5c, 0x12, 0x34,
	0x52, 0xc5, 0xaf, 0x54, 0x2a, 0x8c, 0xa0, 0x47, 0x51, 0xb7, 0x0d, 0x7c, 0x76, 0xff, 0x2e, 0xfa,
	0xcf, 0xf7, 0xf6, 0xcb, 0x98, 0xbc, 0x81, 0xe7, 0xe0, 0x84, 0xd9, 0x62, 0x87, 0x7c, 0x72, 0xc7,
	0xf2, 0xcf, 0xa8, 0xfd, 0xa7, 0x7b, 0xba, 0x15, 0x6e, 0xf4, 0xf8, 0xdb, 0x23, 0x52, 0x0c, 0xcb,
	0xdf, 0xb1, 0x4c, 0x2f, 0x87, 0x57, 0xf9, 0xed, 0x27, 0xb9, 0x3c, 0xa2, 0xf3, 0xdd, 0xdf, 0x00,
	0x00, 0x00, 0xff, 0xff, 0x0f, 0x50, 0xc9, 0xf2, 0x3b, 0x03, 0x00, 0x00,
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConnInterface

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion6

// CallTraceServiceClient is the client API for CallTraceService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
type CallTraceServiceClient interface {
	StartCallTrace(ctx context.Context, in *StartTraceRequest, opts ...grpc.CallOption) (*StartTraceResponse, error)
	EndCallTrace(ctx context.Context, in *EndTraceRequest, opts ...grpc.CallOption) (*EndTraceResponse, error)
}

type callTraceServiceClient struct {
	cc grpc.ClientConnInterface
}

func NewCallTraceServiceClient(cc grpc.ClientConnInterface) CallTraceServiceClient {
	return &callTraceServiceClient{cc}
}

func (c *callTraceServiceClient) StartCallTrace(ctx context.Context, in *StartTraceRequest, opts ...grpc.CallOption) (*StartTraceResponse, error) {
	out := new(StartTraceResponse)
	err := c.cc.Invoke(ctx, "/magma.orc8r.CallTraceService/StartCallTrace", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *callTraceServiceClient) EndCallTrace(ctx context.Context, in *EndTraceRequest, opts ...grpc.CallOption) (*EndTraceResponse, error) {
	out := new(EndTraceResponse)
	err := c.cc.Invoke(ctx, "/magma.orc8r.CallTraceService/EndCallTrace", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// CallTraceServiceServer is the server API for CallTraceService service.
type CallTraceServiceServer interface {
	StartCallTrace(context.Context, *StartTraceRequest) (*StartTraceResponse, error)
	EndCallTrace(context.Context, *EndTraceRequest) (*EndTraceResponse, error)
}

// UnimplementedCallTraceServiceServer can be embedded to have forward compatible implementations.
type UnimplementedCallTraceServiceServer struct {
}

func (*UnimplementedCallTraceServiceServer) StartCallTrace(ctx context.Context, req *StartTraceRequest) (*StartTraceResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method StartCallTrace not implemented")
}
func (*UnimplementedCallTraceServiceServer) EndCallTrace(ctx context.Context, req *EndTraceRequest) (*EndTraceResponse, error) {
	return nil, status.Errorf(codes.Unimplemented, "method EndCallTrace not implemented")
}

func RegisterCallTraceServiceServer(s *grpc.Server, srv CallTraceServiceServer) {
	s.RegisterService(&_CallTraceService_serviceDesc, srv)
}

func _CallTraceService_StartCallTrace_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(StartTraceRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(CallTraceServiceServer).StartCallTrace(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/magma.orc8r.CallTraceService/StartCallTrace",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(CallTraceServiceServer).StartCallTrace(ctx, req.(*StartTraceRequest))
	}
	return interceptor(ctx, in, info, handler)
}

func _CallTraceService_EndCallTrace_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(EndTraceRequest)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(CallTraceServiceServer).EndCallTrace(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/magma.orc8r.CallTraceService/EndCallTrace",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(CallTraceServiceServer).EndCallTrace(ctx, req.(*EndTraceRequest))
	}
	return interceptor(ctx, in, info, handler)
}

var _CallTraceService_serviceDesc = grpc.ServiceDesc{
	ServiceName: "magma.orc8r.CallTraceService",
	HandlerType: (*CallTraceServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "StartCallTrace",
			Handler:    _CallTraceService_StartCallTrace_Handler,
		},
		{
			MethodName: "EndCallTrace",
			Handler:    _CallTraceService_EndCallTrace_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "orc8r/protos/ctraced.proto",
}
