// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "bucketlistmerger.h"
#include "distributor_stripe_component.h"
#include "distributormessagesender.h"
#include "messageguard.h"
#include "operation_routing_snapshot.h"
#include "outdated_nodes_map.h"
#include "pendingclusterstate.h"
#include <vespa/document/bucket/bucket.h>
#include <vespa/storage/common/storagelink.h>
#include <vespa/storageapi/message/bucket.h>
#include <vespa/storageapi/messageapi/messagehandler.h>
#include <vespa/storageframework/generic/clock/timer.h>
#include <vespa/storageframework/generic/status/statusreporter.h>
#include <vespa/vdslib/state/clusterstate.h>
#include <atomic>
#include <list>
#include <mutex>

namespace vespalib::xml {
class XmlOutputStream;
class XmlAttribute;
}

namespace storage::distributor {

struct BucketSpaceDistributionConfigs;
class BucketSpaceDistributionContext;
class DistributorInterface;
class StripeAccessor;
class StripeAccessGuard;

class BucketDBUpdater : public framework::StatusReporter,
                        public api::MessageHandler
{
public:
    using OutdatedNodesMap = dbtransition::OutdatedNodesMap;
    BucketDBUpdater(const DistributorNodeContext& node_ctx,
                    DistributorOperationContext& op_ctx,
                    DistributorInterface& distributor_interface,
                    DistributorMessageSender& sender,
                    ChainedMessageSender& chained_sender,
                    std::shared_ptr<const lib::Distribution> bootstrap_distribution,
                    StripeAccessor& stripe_accessor);
    ~BucketDBUpdater() override;

    void flush();

    bool onSetSystemState(const std::shared_ptr<api::SetSystemStateCommand>& cmd) override;
    bool onActivateClusterStateVersion(const std::shared_ptr<api::ActivateClusterStateVersionCommand>& cmd) override;
    bool onRequestBucketInfoReply(const std::shared_ptr<api::RequestBucketInfoReply> & repl) override;

    vespalib::string getReportContentType(const framework::HttpUrlPath&) const override;
    bool reportStatus(std::ostream&, const framework::HttpUrlPath&) const override;

    void resend_delayed_messages();
    void storage_distribution_changed(const BucketSpaceDistributionConfigs& configs);
    void bootstrap_distribution_config(std::shared_ptr<const lib::Distribution>);

    vespalib::string report_xml_status(vespalib::xml::XmlOutputStream& xos, const framework::HttpUrlPath&) const;

    void print(std::ostream& out, bool verbose, const std::string& indent) const;

    void set_stale_reads_enabled(bool enabled) noexcept {
        _stale_reads_enabled.store(enabled, std::memory_order_relaxed);
    }
    bool stale_reads_enabled() const noexcept {
        return _stale_reads_enabled.load(std::memory_order_relaxed);
    }

private:

    friend class DistributorTestUtil;
    // Only to be used by tests that want to ensure both the BucketDBUpdater _and_ the Distributor
    // components agree on the currently active cluster state bundle.
    // Transitively invokes Distributor::enableClusterStateBundle
    void simulate_cluster_state_bundle_activation(const lib::ClusterStateBundle& activated_state);

    bool should_defer_state_enabling() const noexcept;
    bool has_pending_cluster_state() const;
    bool pending_cluster_state_accepted(const std::shared_ptr<api::RequestBucketInfoReply>& repl);
    bool is_pending_cluster_state_completed() const;
    void process_completed_pending_cluster_state(StripeAccessGuard& guard);
    void activate_pending_cluster_state(StripeAccessGuard& guard);
    void ensure_transition_timer_started();
    void complete_transition_timer();

    void remove_superfluous_buckets(StripeAccessGuard& guard,
                                    const lib::ClusterStateBundle& new_state,
                                    bool is_distribution_config_change);

    void reply_to_previous_pending_cluster_state_if_any();
    void reply_to_activation_with_actual_version(
            const api::ActivateClusterStateVersionCommand& cmd,
            uint32_t actualVersion);

    void enable_current_cluster_state_bundle_in_distributor_and_stripes(StripeAccessGuard& guard);
    void add_current_state_to_cluster_state_history();

    void propagate_active_state_bundle_internally();

    void maybe_inject_simulated_db_pruning_delay();
    void maybe_inject_simulated_db_merging_delay();

    // TODO STRIPE remove once distributor component dependencies have been pruned
    StripeAccessor& _stripe_accessor;
    lib::ClusterStateBundle _active_state_bundle;

    const DistributorNodeContext& _node_ctx;
    DistributorOperationContext& _op_ctx;
    DistributorInterface& _distributor_interface;
    std::unique_ptr<PendingClusterState> _pending_cluster_state;
    std::list<PendingClusterState::Summary> _history;
    DistributorMessageSender& _sender;
    ChainedMessageSender& _chained_sender;
    OutdatedNodesMap         _outdated_nodes_map;
    framework::MilliSecTimer _transition_timer;
    std::atomic<bool> _stale_reads_enabled;
};

}
