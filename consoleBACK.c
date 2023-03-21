void consoleHandlerDeferred(void) {
    consoleInterrupt.serviced++;

    struct xencons_interface *intf = xencons_interface();
    XENCONS_RING_IDX cons, prod;
    cons = intf->in_cons;
    prod = intf->in_prod;
    mb();
    BUG_ON((prod - cons) > sizeof(intf->in));
    while (cons != prod) {
        xencons_rx(intf->in + MASK_XENCONS_IDX(cons, intf->in), 1);
        cons++;
    }
    mb();
    intf->in_cons = cons;
    xencons_notify_backend();
    xencons_tx();

    consoleDoAll();
}