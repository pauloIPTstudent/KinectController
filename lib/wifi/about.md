# Documentação da biblioteca


### Inicialização basica para utilizar as funções wifi
- void wifi_init();
- void wifi_init_event_handler();
- void wifi_register_callbacks(wifi_event_callbacks_t *callbacks);

### Começar conexão wifi (STA,AP,STA/AP)
- void wifi_start_sta(wifi_sta_config_t *config);
- void wifi_start_ap(wifi_ap_config_t *config);
- void wifi_start_ap_sta(wifi_sta_config_t *sta, wifi_ap_config_t *ap);

### Parar a respectiva conecção (STA,AP,STA/AP)
- void wifi_stop_ap();
- void wifi_stop_sta();
- void wifi_stop_ap_sta();

### Helpers
- bool wifi_is_connected();