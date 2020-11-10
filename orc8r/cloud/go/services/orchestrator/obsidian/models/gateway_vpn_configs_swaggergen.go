// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	strfmt "github.com/go-openapi/strfmt"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"
)

// GatewayVpnConfigs Configuration for gateway VPN access
// swagger:model gateway_vpn_configs
type GatewayVpnConfigs struct {

	// enable shell
	// Required: true
	EnableShell *bool `json:"enable_shell"`
}

// Validate validates this gateway vpn configs
func (m *GatewayVpnConfigs) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateEnableShell(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *GatewayVpnConfigs) validateEnableShell(formats strfmt.Registry) error {

	if err := validate.Required("enable_shell", "body", m.EnableShell); err != nil {
		return err
	}

	return nil
}

// MarshalBinary interface implementation
func (m *GatewayVpnConfigs) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *GatewayVpnConfigs) UnmarshalBinary(b []byte) error {
	var res GatewayVpnConfigs
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
