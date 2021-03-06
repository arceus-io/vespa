// Copyright 2019 Oath Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.provision.provisioning;

import com.yahoo.config.provision.Flavor;
import com.yahoo.vespa.hosted.provision.Node;

import java.util.List;
import java.util.Set;

/**
 * Service for provisioning physical docker tenant hosts inside the zone.
 *
 * @author freva
 */
public interface HostProvisioner {

    /**
     * Schedule provisioning of a given number of hosts.
     *
     * @param provisionIndexes List of unique provision indexes which will be used to generate the host hostnames
     *                         on the form of <code>[prefix][index].[domain]</code>
     * @param nodeFlavor Vespa flavor of the node that will run on this host. The resulting provisioned host
     *                   will be of a flavor that is at least as big or bigger than this.
     * @return list of {@link ProvisionedHost} describing the provisioned hosts and nodes on them.
     */
    List<ProvisionedHost> provisionHosts(List<Integer> provisionIndexes, Flavor nodeFlavor);

    /**
     * Continue provisioning of given list of Nodes.
     *
     * @param host the host to provision
     * @param children list of all the nodes that run on the given host
     * @return a subset of {@code host} and {@code children} where the values have been modified and should
     * be written back to node-repository.
     * @throws FatalProvisioningException if the provisioning has irrecoverably failed and the input nodes
     * should be deleted from node-repo.
     */
    List<Node> provision(Node host, Set<Node> children) throws FatalProvisioningException;

    /**
     * Deprovisions a given host and resources associated with it and its children (such as DNS entries).
     * This method will only perform the actual deprovisioning of the host and does NOT:
     *  - verify whether it is safe to do
     *  - clean up config server references to this node or any of its children
     * Therefore, this method should probably only be called for hosts that have no children.
     *
     * @param host host to deprovision.
     */
    void deprovision(Node host);
}
