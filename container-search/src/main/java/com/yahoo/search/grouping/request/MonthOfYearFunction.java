// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.grouping.request;

import java.util.Arrays;

/**
 * This class represents a month-of-year timestamp-function in a {@link GroupingExpression}. It evaluates to a long that
 * equals the month of year (1-12) of the result of the argument.
 *
 * @author <a href="mailto:simon@yahoo-inc.com">Simon Thoresen</a>
 */
public class MonthOfYearFunction extends FunctionNode {

    /**
     * Constructs a new instance of this class.
     *
     * @param exp The expression to evaluate, must evaluate to a long.
     */
    public MonthOfYearFunction(GroupingExpression exp) {
        super("time.monthofyear", Arrays.asList(exp));
    }
}
