# Management Agents

The management agents component is concerned with providing agents that interact with external applications or services for performing management operations.  Agents for remote and local management are required as follows:

 * Remote device management via Mbed Cloud - a cloud management agent brokers LwM2M object requests to suitable handlers.  Device health notifications and information will be provided to the cloud via the Mbed Connect service.  LwM2M defines a set of objects for advertising device information and for triggering actions.  Requests will be brokered to concrete handlers to perform the actual device management operations.
 * Local device setup via BLE - to allow for local device setup, a management agent will present a BLE interface to allow a local operator to manage settings such as the Wi-Fi or general network configuration.  In a similar manner to the handling of requests received via the Mbed Connect service, BLE requests will be brokered to concrete handlers that perform network setup and other device configuration.
 * Local device setup via other interfaces - other setup agents are likely to be needed such as Wi-Fi Direct or USB.


## License

Please see the [License][mbl-license] document for more information.


## Contributing

Please see the [Contributing][mbl-contributing] document for more information.


[mbl-license]: ../LICENSE.md
[mbl-contributing]: ../CONTRIBUTING.md
