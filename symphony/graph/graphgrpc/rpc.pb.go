// Copyright (c) 2004-present Facebook All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Code generated by protoc-gen-go. DO NOT EDIT.
// source: rpc.proto

package graphgrpc

import (
	context "context"
	fmt "fmt"
	math "math"

	proto "github.com/golang/protobuf/proto"
	empty "github.com/golang/protobuf/ptypes/empty"
	wrappers "github.com/golang/protobuf/ptypes/wrappers"
	grpc "google.golang.org/grpc"
	codes "google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
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

type Tenant struct {
	Id                   string   `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	Name                 string   `protobuf:"bytes,2,opt,name=name,proto3" json:"name,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *Tenant) Reset()         { *m = Tenant{} }
func (m *Tenant) String() string { return proto.CompactTextString(m) }
func (*Tenant) ProtoMessage()    {}
func (*Tenant) Descriptor() ([]byte, []int) {
	return fileDescriptor_77a6da22d6a3feb1, []int{0}
}

func (m *Tenant) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_Tenant.Unmarshal(m, b)
}
func (m *Tenant) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_Tenant.Marshal(b, m, deterministic)
}
func (m *Tenant) XXX_Merge(src proto.Message) {
	xxx_messageInfo_Tenant.Merge(m, src)
}
func (m *Tenant) XXX_Size() int {
	return xxx_messageInfo_Tenant.Size(m)
}
func (m *Tenant) XXX_DiscardUnknown() {
	xxx_messageInfo_Tenant.DiscardUnknown(m)
}

var xxx_messageInfo_Tenant proto.InternalMessageInfo

func (m *Tenant) GetId() string {
	if m != nil {
		return m.Id
	}
	return ""
}

func (m *Tenant) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

type TenantList struct {
	Tenants              []*Tenant `protobuf:"bytes,1,rep,name=tenants,proto3" json:"tenants,omitempty"`
	XXX_NoUnkeyedLiteral struct{}  `json:"-"`
	XXX_unrecognized     []byte    `json:"-"`
	XXX_sizecache        int32     `json:"-"`
}

func (m *TenantList) Reset()         { *m = TenantList{} }
func (m *TenantList) String() string { return proto.CompactTextString(m) }
func (*TenantList) ProtoMessage()    {}
func (*TenantList) Descriptor() ([]byte, []int) {
	return fileDescriptor_77a6da22d6a3feb1, []int{1}
}

func (m *TenantList) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_TenantList.Unmarshal(m, b)
}
func (m *TenantList) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_TenantList.Marshal(b, m, deterministic)
}
func (m *TenantList) XXX_Merge(src proto.Message) {
	xxx_messageInfo_TenantList.Merge(m, src)
}
func (m *TenantList) XXX_Size() int {
	return xxx_messageInfo_TenantList.Size(m)
}
func (m *TenantList) XXX_DiscardUnknown() {
	xxx_messageInfo_TenantList.DiscardUnknown(m)
}

var xxx_messageInfo_TenantList proto.InternalMessageInfo

func (m *TenantList) GetTenants() []*Tenant {
	if m != nil {
		return m.Tenants
	}
	return nil
}

type AlertPayload struct {
	TenantID             string            `protobuf:"bytes,1,opt,name=tenantID,proto3" json:"tenantID,omitempty"`
	Alertname            string            `protobuf:"bytes,2,opt,name=alertname,proto3" json:"alertname,omitempty"`
	NetworkID            string            `protobuf:"bytes,3,opt,name=networkID,proto3" json:"networkID,omitempty"`
	Labels               map[string]string `protobuf:"bytes,4,rep,name=labels,proto3" json:"labels,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	XXX_NoUnkeyedLiteral struct{}          `json:"-"`
	XXX_unrecognized     []byte            `json:"-"`
	XXX_sizecache        int32             `json:"-"`
}

func (m *AlertPayload) Reset()         { *m = AlertPayload{} }
func (m *AlertPayload) String() string { return proto.CompactTextString(m) }
func (*AlertPayload) ProtoMessage()    {}
func (*AlertPayload) Descriptor() ([]byte, []int) {
	return fileDescriptor_77a6da22d6a3feb1, []int{2}
}

func (m *AlertPayload) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_AlertPayload.Unmarshal(m, b)
}
func (m *AlertPayload) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_AlertPayload.Marshal(b, m, deterministic)
}
func (m *AlertPayload) XXX_Merge(src proto.Message) {
	xxx_messageInfo_AlertPayload.Merge(m, src)
}
func (m *AlertPayload) XXX_Size() int {
	return xxx_messageInfo_AlertPayload.Size(m)
}
func (m *AlertPayload) XXX_DiscardUnknown() {
	xxx_messageInfo_AlertPayload.DiscardUnknown(m)
}

var xxx_messageInfo_AlertPayload proto.InternalMessageInfo

func (m *AlertPayload) GetTenantID() string {
	if m != nil {
		return m.TenantID
	}
	return ""
}

func (m *AlertPayload) GetAlertname() string {
	if m != nil {
		return m.Alertname
	}
	return ""
}

func (m *AlertPayload) GetNetworkID() string {
	if m != nil {
		return m.NetworkID
	}
	return ""
}

func (m *AlertPayload) GetLabels() map[string]string {
	if m != nil {
		return m.Labels
	}
	return nil
}

type ExecutionResult struct {
	Successes            []string          `protobuf:"bytes,1,rep,name=successes,proto3" json:"successes,omitempty"`
	Errors               []*ExecutionError `protobuf:"bytes,2,rep,name=errors,proto3" json:"errors,omitempty"`
	XXX_NoUnkeyedLiteral struct{}          `json:"-"`
	XXX_unrecognized     []byte            `json:"-"`
	XXX_sizecache        int32             `json:"-"`
}

func (m *ExecutionResult) Reset()         { *m = ExecutionResult{} }
func (m *ExecutionResult) String() string { return proto.CompactTextString(m) }
func (*ExecutionResult) ProtoMessage()    {}
func (*ExecutionResult) Descriptor() ([]byte, []int) {
	return fileDescriptor_77a6da22d6a3feb1, []int{3}
}

func (m *ExecutionResult) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_ExecutionResult.Unmarshal(m, b)
}
func (m *ExecutionResult) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_ExecutionResult.Marshal(b, m, deterministic)
}
func (m *ExecutionResult) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ExecutionResult.Merge(m, src)
}
func (m *ExecutionResult) XXX_Size() int {
	return xxx_messageInfo_ExecutionResult.Size(m)
}
func (m *ExecutionResult) XXX_DiscardUnknown() {
	xxx_messageInfo_ExecutionResult.DiscardUnknown(m)
}

var xxx_messageInfo_ExecutionResult proto.InternalMessageInfo

func (m *ExecutionResult) GetSuccesses() []string {
	if m != nil {
		return m.Successes
	}
	return nil
}

func (m *ExecutionResult) GetErrors() []*ExecutionError {
	if m != nil {
		return m.Errors
	}
	return nil
}

type ExecutionError struct {
	Id                   string   `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	Err                  string   `protobuf:"bytes,2,opt,name=err,proto3" json:"err,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *ExecutionError) Reset()         { *m = ExecutionError{} }
func (m *ExecutionError) String() string { return proto.CompactTextString(m) }
func (*ExecutionError) ProtoMessage()    {}
func (*ExecutionError) Descriptor() ([]byte, []int) {
	return fileDescriptor_77a6da22d6a3feb1, []int{4}
}

func (m *ExecutionError) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_ExecutionError.Unmarshal(m, b)
}
func (m *ExecutionError) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_ExecutionError.Marshal(b, m, deterministic)
}
func (m *ExecutionError) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ExecutionError.Merge(m, src)
}
func (m *ExecutionError) XXX_Size() int {
	return xxx_messageInfo_ExecutionError.Size(m)
}
func (m *ExecutionError) XXX_DiscardUnknown() {
	xxx_messageInfo_ExecutionError.DiscardUnknown(m)
}

var xxx_messageInfo_ExecutionError proto.InternalMessageInfo

func (m *ExecutionError) GetId() string {
	if m != nil {
		return m.Id
	}
	return ""
}

func (m *ExecutionError) GetErr() string {
	if m != nil {
		return m.Err
	}
	return ""
}

func init() {
	proto.RegisterType((*Tenant)(nil), "graph.Tenant")
	proto.RegisterType((*TenantList)(nil), "graph.TenantList")
	proto.RegisterType((*AlertPayload)(nil), "graph.AlertPayload")
	proto.RegisterMapType((map[string]string)(nil), "graph.AlertPayload.LabelsEntry")
	proto.RegisterType((*ExecutionResult)(nil), "graph.ExecutionResult")
	proto.RegisterType((*ExecutionError)(nil), "graph.executionError")
}

func init() { proto.RegisterFile("rpc.proto", fileDescriptor_77a6da22d6a3feb1) }

var fileDescriptor_77a6da22d6a3feb1 = []byte{
	// 481 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x9c, 0x52, 0x5d, 0x4f, 0xd4, 0x40,
	0x14, 0x4d, 0xdb, 0xa5, 0xb0, 0x17, 0x41, 0x1c, 0xd4, 0x34, 0x75, 0x83, 0xa4, 0x2f, 0xf2, 0xa0,
	0xdd, 0x58, 0x83, 0x5f, 0x0f, 0x46, 0x74, 0x37, 0x86, 0x04, 0x13, 0x53, 0x36, 0x3e, 0xf8, 0x60,
	0x32, 0xdb, 0xbd, 0x94, 0x86, 0x6e, 0xdb, 0xdc, 0x99, 0x82, 0xfd, 0x55, 0xfe, 0x1f, 0x7f, 0x8d,
	0x99, 0x99, 0x16, 0x76, 0x41, 0x63, 0xe4, 0xa5, 0x99, 0x7b, 0xce, 0x99, 0xb9, 0xa7, 0xf7, 0x1e,
	0xe8, 0x53, 0x95, 0x84, 0x15, 0x95, 0xb2, 0x64, 0x2b, 0x29, 0xf1, 0xea, 0xd4, 0x7f, 0x94, 0x96,
	0x65, 0x9a, 0xe3, 0x50, 0x83, 0xd3, 0xfa, 0x64, 0x88, 0xf3, 0x4a, 0x36, 0x46, 0xe3, 0xef, 0x5c,
	0x27, 0x2f, 0x88, 0x57, 0x15, 0x92, 0x30, 0x7c, 0xf0, 0x14, 0xdc, 0x09, 0x16, 0xbc, 0x90, 0x6c,
	0x13, 0xec, 0x6c, 0xe6, 0x59, 0xbb, 0xd6, 0x5e, 0x3f, 0xb6, 0xb3, 0x19, 0x63, 0xd0, 0x2b, 0xf8,
	0x1c, 0x3d, 0x5b, 0x23, 0xfa, 0x1c, 0xec, 0x03, 0x18, 0xf5, 0x51, 0x26, 0x24, 0x7b, 0x02, 0xab,
	0x52, 0x57, 0xc2, 0xb3, 0x76, 0x9d, 0xbd, 0xf5, 0x68, 0x23, 0xd4, 0x8e, 0x42, 0xa3, 0x89, 0x3b,
	0x36, 0xf8, 0x65, 0xc1, 0x9d, 0x83, 0x1c, 0x49, 0x7e, 0xe1, 0x4d, 0x5e, 0xf2, 0x19, 0xf3, 0x61,
	0xcd, 0x70, 0x87, 0xa3, 0xb6, 0xe3, 0x65, 0xcd, 0x06, 0xd0, 0xe7, 0x4a, 0xbb, 0xd0, 0xfc, 0x0a,
	0x50, 0x6c, 0x81, 0xf2, 0xa2, 0xa4, 0xb3, 0xc3, 0x91, 0xe7, 0x18, 0xf6, 0x12, 0x60, 0xaf, 0xc0,
	0xcd, 0xf9, 0x14, 0x73, 0xe1, 0xf5, 0xb4, 0xa1, 0xc7, 0xad, 0xa1, 0xc5, 0xe6, 0xe1, 0x91, 0x56,
	0x8c, 0x0b, 0x49, 0x4d, 0xdc, 0xca, 0xfd, 0x37, 0xb0, 0xbe, 0x00, 0xb3, 0x2d, 0x70, 0xce, 0xb0,
	0x69, 0xad, 0xa9, 0x23, 0xbb, 0x0f, 0x2b, 0xe7, 0x3c, 0xaf, 0x3b, 0x47, 0xa6, 0x78, 0x6b, 0xbf,
	0xb6, 0x82, 0xef, 0x70, 0x77, 0xfc, 0x03, 0x93, 0x5a, 0x66, 0x65, 0x11, 0xa3, 0xa8, 0x73, 0xa9,
	0x4c, 0x8a, 0x3a, 0x49, 0x50, 0x08, 0x34, 0xa3, 0xe9, 0xc7, 0x57, 0x00, 0x7b, 0x06, 0x2e, 0x12,
	0x95, 0x24, 0x3c, 0x5b, 0x9b, 0x7c, 0xd0, 0x9a, 0xc4, 0xee, 0x95, 0xb1, 0x62, 0xe3, 0x56, 0x14,
	0x44, 0xb0, 0xb9, 0xcc, 0xdc, 0xd8, 0xd4, 0x16, 0x38, 0x48, 0xd4, 0x3a, 0x53, 0xc7, 0xe8, 0xa7,
	0x0d, 0x1b, 0x66, 0x09, 0xc7, 0x48, 0xe7, 0x59, 0x82, 0x6c, 0x1f, 0xdc, 0x8f, 0x84, 0x5c, 0x22,
	0x1b, 0x84, 0x26, 0x12, 0x61, 0x17, 0x89, 0xf0, 0x58, 0x52, 0x56, 0xa4, 0x5f, 0xd5, 0x1f, 0xf9,
	0xcb, 0x2b, 0x64, 0xcf, 0xa1, 0xa7, 0x57, 0xfd, 0xf0, 0xc6, 0xa5, 0xb1, 0x0a, 0x99, 0x7f, 0x6f,
	0x49, 0xae, 0xa5, 0x11, 0x38, 0x9f, 0x50, 0xfe, 0x5f, 0x9b, 0xf7, 0xb0, 0x36, 0xa1, 0xba, 0x48,
	0xfe, 0xed, 0xef, 0x2f, 0x46, 0xd8, 0x3b, 0x70, 0x47, 0x98, 0xe3, 0x6d, 0xef, 0x47, 0x9f, 0x61,
	0xfb, 0x20, 0x51, 0x23, 0x16, 0x3a, 0x2b, 0xdd, 0xd8, 0x5e, 0xc2, 0xea, 0x84, 0xb2, 0x34, 0x45,
	0x62, 0xdb, 0x7f, 0xc8, 0x92, 0x7a, 0x4e, 0x83, 0xd7, 0x12, 0xf0, 0x61, 0xe7, 0xdb, 0xe0, 0x64,
	0x9a, 0x0c, 0x45, 0x33, 0xaf, 0x4e, 0xcb, 0xa2, 0x19, 0x6a, 0x95, 0xf9, 0xa6, 0x54, 0x25, 0x53,
	0x57, 0xb7, 0x7f, 0xf1, 0x3b, 0x00, 0x00, 0xff, 0xff, 0x93, 0x0e, 0xd2, 0x9b, 0xce, 0x03, 0x00,
	0x00,
}

// Reference imports to suppress errors if they are not otherwise used.
var _ context.Context
var _ grpc.ClientConn

// This is a compile-time assertion to ensure that this generated file
// is compatible with the grpc package it is being compiled against.
const _ = grpc.SupportPackageIsVersion4

// TenantServiceClient is the client API for TenantService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
type TenantServiceClient interface {
	Create(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*Tenant, error)
	List(ctx context.Context, in *empty.Empty, opts ...grpc.CallOption) (*TenantList, error)
	Get(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*Tenant, error)
	Truncate(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*empty.Empty, error)
	Delete(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*empty.Empty, error)
}

type tenantServiceClient struct {
	cc *grpc.ClientConn
}

func NewTenantServiceClient(cc *grpc.ClientConn) TenantServiceClient {
	return &tenantServiceClient{cc}
}

func (c *tenantServiceClient) Create(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*Tenant, error) {
	out := new(Tenant)
	err := c.cc.Invoke(ctx, "/graph.TenantService/Create", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *tenantServiceClient) List(ctx context.Context, in *empty.Empty, opts ...grpc.CallOption) (*TenantList, error) {
	out := new(TenantList)
	err := c.cc.Invoke(ctx, "/graph.TenantService/List", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *tenantServiceClient) Get(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*Tenant, error) {
	out := new(Tenant)
	err := c.cc.Invoke(ctx, "/graph.TenantService/Get", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *tenantServiceClient) Truncate(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*empty.Empty, error) {
	out := new(empty.Empty)
	err := c.cc.Invoke(ctx, "/graph.TenantService/Truncate", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

func (c *tenantServiceClient) Delete(ctx context.Context, in *wrappers.StringValue, opts ...grpc.CallOption) (*empty.Empty, error) {
	out := new(empty.Empty)
	err := c.cc.Invoke(ctx, "/graph.TenantService/Delete", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// TenantServiceServer is the server API for TenantService service.
type TenantServiceServer interface {
	Create(context.Context, *wrappers.StringValue) (*Tenant, error)
	List(context.Context, *empty.Empty) (*TenantList, error)
	Get(context.Context, *wrappers.StringValue) (*Tenant, error)
	Truncate(context.Context, *wrappers.StringValue) (*empty.Empty, error)
	Delete(context.Context, *wrappers.StringValue) (*empty.Empty, error)
}

// UnimplementedTenantServiceServer can be embedded to have forward compatible implementations.
type UnimplementedTenantServiceServer struct {
}

func (*UnimplementedTenantServiceServer) Create(ctx context.Context, req *wrappers.StringValue) (*Tenant, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Create not implemented")
}
func (*UnimplementedTenantServiceServer) List(ctx context.Context, req *empty.Empty) (*TenantList, error) {
	return nil, status.Errorf(codes.Unimplemented, "method List not implemented")
}
func (*UnimplementedTenantServiceServer) Get(ctx context.Context, req *wrappers.StringValue) (*Tenant, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Get not implemented")
}
func (*UnimplementedTenantServiceServer) Truncate(ctx context.Context, req *wrappers.StringValue) (*empty.Empty, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Truncate not implemented")
}
func (*UnimplementedTenantServiceServer) Delete(ctx context.Context, req *wrappers.StringValue) (*empty.Empty, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Delete not implemented")
}

func RegisterTenantServiceServer(s *grpc.Server, srv TenantServiceServer) {
	s.RegisterService(&_TenantService_serviceDesc, srv)
}

func _TenantService_Create_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(wrappers.StringValue)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TenantServiceServer).Create(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.TenantService/Create",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TenantServiceServer).Create(ctx, req.(*wrappers.StringValue))
	}
	return interceptor(ctx, in, info, handler)
}

func _TenantService_List_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(empty.Empty)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TenantServiceServer).List(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.TenantService/List",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TenantServiceServer).List(ctx, req.(*empty.Empty))
	}
	return interceptor(ctx, in, info, handler)
}

func _TenantService_Get_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(wrappers.StringValue)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TenantServiceServer).Get(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.TenantService/Get",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TenantServiceServer).Get(ctx, req.(*wrappers.StringValue))
	}
	return interceptor(ctx, in, info, handler)
}

func _TenantService_Truncate_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(wrappers.StringValue)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TenantServiceServer).Truncate(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.TenantService/Truncate",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TenantServiceServer).Truncate(ctx, req.(*wrappers.StringValue))
	}
	return interceptor(ctx, in, info, handler)
}

func _TenantService_Delete_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(wrappers.StringValue)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(TenantServiceServer).Delete(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.TenantService/Delete",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(TenantServiceServer).Delete(ctx, req.(*wrappers.StringValue))
	}
	return interceptor(ctx, in, info, handler)
}

var _TenantService_serviceDesc = grpc.ServiceDesc{
	ServiceName: "graph.TenantService",
	HandlerType: (*TenantServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "Create",
			Handler:    _TenantService_Create_Handler,
		},
		{
			MethodName: "List",
			Handler:    _TenantService_List_Handler,
		},
		{
			MethodName: "Get",
			Handler:    _TenantService_Get_Handler,
		},
		{
			MethodName: "Truncate",
			Handler:    _TenantService_Truncate_Handler,
		},
		{
			MethodName: "Delete",
			Handler:    _TenantService_Delete_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "rpc.proto",
}

// ActionsAlertServiceClient is the client API for ActionsAlertService service.
//
// For semantics around ctx use and closing/ending streaming RPCs, please refer to https://godoc.org/google.golang.org/grpc#ClientConn.NewStream.
type ActionsAlertServiceClient interface {
	Trigger(ctx context.Context, in *AlertPayload, opts ...grpc.CallOption) (*ExecutionResult, error)
}

type actionsAlertServiceClient struct {
	cc *grpc.ClientConn
}

func NewActionsAlertServiceClient(cc *grpc.ClientConn) ActionsAlertServiceClient {
	return &actionsAlertServiceClient{cc}
}

func (c *actionsAlertServiceClient) Trigger(ctx context.Context, in *AlertPayload, opts ...grpc.CallOption) (*ExecutionResult, error) {
	out := new(ExecutionResult)
	err := c.cc.Invoke(ctx, "/graph.ActionsAlertService/Trigger", in, out, opts...)
	if err != nil {
		return nil, err
	}
	return out, nil
}

// ActionsAlertServiceServer is the server API for ActionsAlertService service.
type ActionsAlertServiceServer interface {
	Trigger(context.Context, *AlertPayload) (*ExecutionResult, error)
}

// UnimplementedActionsAlertServiceServer can be embedded to have forward compatible implementations.
type UnimplementedActionsAlertServiceServer struct {
}

func (*UnimplementedActionsAlertServiceServer) Trigger(ctx context.Context, req *AlertPayload) (*ExecutionResult, error) {
	return nil, status.Errorf(codes.Unimplemented, "method Trigger not implemented")
}

func RegisterActionsAlertServiceServer(s *grpc.Server, srv ActionsAlertServiceServer) {
	s.RegisterService(&_ActionsAlertService_serviceDesc, srv)
}

func _ActionsAlertService_Trigger_Handler(srv interface{}, ctx context.Context, dec func(interface{}) error, interceptor grpc.UnaryServerInterceptor) (interface{}, error) {
	in := new(AlertPayload)
	if err := dec(in); err != nil {
		return nil, err
	}
	if interceptor == nil {
		return srv.(ActionsAlertServiceServer).Trigger(ctx, in)
	}
	info := &grpc.UnaryServerInfo{
		Server:     srv,
		FullMethod: "/graph.ActionsAlertService/Trigger",
	}
	handler := func(ctx context.Context, req interface{}) (interface{}, error) {
		return srv.(ActionsAlertServiceServer).Trigger(ctx, req.(*AlertPayload))
	}
	return interceptor(ctx, in, info, handler)
}

var _ActionsAlertService_serviceDesc = grpc.ServiceDesc{
	ServiceName: "graph.ActionsAlertService",
	HandlerType: (*ActionsAlertServiceServer)(nil),
	Methods: []grpc.MethodDesc{
		{
			MethodName: "Trigger",
			Handler:    _ActionsAlertService_Trigger_Handler,
		},
	},
	Streams:  []grpc.StreamDesc{},
	Metadata: "rpc.proto",
}
