// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.metrics;

public abstract class MetricVisitor {
    /**
     * Visit a metric set.
     *
     * @param autoGenerated True for metric sets that are generated on the
     *                      fly such as in sum metrics.
     * @return True if you want to visit the content of this metric set.
     */
    public boolean visitMetricSet(MetricSet set, boolean autoGenerated) {
        return true;
    }

    /**
     * Callback visitors can use if they need to know the tree traversal of
     * metric sets. This function is not called if visitMetricSet returned
     * false.
     */
    public void doneVisitingMetricSet(MetricSet set) {
    }

    /**
     * Visit a primitive metric within an accepted metric set.
     *
     * @return True if you want to continue visiting, false to abort.
     */
    public boolean visitPrimitiveMetric(Metric m, boolean autoGenerated) {
        return true;
    }
}
