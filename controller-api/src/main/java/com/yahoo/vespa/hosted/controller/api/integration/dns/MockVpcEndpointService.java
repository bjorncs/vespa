package com.yahoo.vespa.hosted.controller.api.integration.dns;

import ai.vespa.http.DomainName;
import com.yahoo.config.provision.CloudAccount;
import com.yahoo.vespa.hosted.controller.api.identifiers.ClusterId;

import java.util.Optional;

/**
 * @author jonmv
 */
public class MockVpcEndpointService implements VpcEndpointService {

    public static final VpcEndpointService empty = (name, cluster, account) -> Optional.empty();

    public VpcEndpointService delegate = empty;

    @Override
    public Optional<DnsChallenge> setPrivateDns(DomainName privateDnsName, ClusterId clusterId, Optional<CloudAccount> account) {
        return delegate.setPrivateDns(privateDnsName, clusterId, account);
    }

}