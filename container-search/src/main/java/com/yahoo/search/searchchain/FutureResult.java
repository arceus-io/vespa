// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.searchchain;

import java.util.Optional;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.yahoo.yolean.Exceptions;
import com.yahoo.search.Query;
import com.yahoo.search.Result;
import com.yahoo.search.result.ErrorMessage;

/**
 * Extends a {@code FutureTask<Result>}, with some added error handling
 * 
 * @author bratseth
 */
public class FutureResult extends FutureTask<Result> {

    private final Query query;

    /** Only used for generating messages */
    private final Execution execution;

    private final static Logger log = Logger.getLogger(FutureResult.class.getName());

    protected FutureResult(Callable<Result> callable, Execution execution, Query query) {
        super(callable);
        this.query = query;
        this.execution = execution;
    }

    /** 
     * Returns a Result containing the hits returned from this source, or an error otherwise.
     * This will block for however long it takes to get the result: Using this is a bad idea.
     */
    @Override
    public Result get() {
        try {
            return super.get();
        }
        catch (InterruptedException e) {
            return new Result(getQuery(), createInterruptedError(e));
        }
        catch (ExecutionException e) {
            return new Result(getQuery(), createExecutionError(e));
        }
    }

    /** 
     * Returns a Result containing the hits returned from this source, or an error otherwise.
     * This blocks for at most the given timeout and returns a Result containing a timeout error
     * if the result is not available within this time.
     */
    @Override
    public Result get(long timeout, TimeUnit timeunit) {
        return getIfAvailable(timeout, timeunit).orElse(new Result(getQuery(), createTimeoutError()));
    }

    /**
     * Same as get(timeout, timeunit) but returns Optiona.empty instead of a result with error if the result is 
     * not available in time
     */
    public Optional<Result> getIfAvailable(long timeout, TimeUnit timeunit) {
        try {
            return Optional.of(super.get(timeout, timeunit));
        }
        catch (InterruptedException e) {
            return Optional.of(new Result(getQuery(), createInterruptedError(e)));
        }
        catch (ExecutionException e) {
            return Optional.of(new Result(getQuery(), createExecutionError(e)));
        }
        catch (TimeoutException e) {
            return Optional.empty();
        }
    }

    /** Returns the query used in this execution, never null */
    public Query getQuery() {
        return query;
    }

    private ErrorMessage createInterruptedError(Exception e) {
        return ErrorMessage.createUnspecifiedError("'" + execution + "' was interrupted while executing: " + 
                                                   Exceptions.toMessageString(e));
    }
    
    private ErrorMessage createExecutionError(Exception e) {
        log.log(Level.WARNING,"Exception on executing " + execution + " for " + query,e);
        return ErrorMessage.createErrorInPluginSearcher("Error in '" + execution + "': " + Exceptions.toMessageString(e),
                                                        e.getCause());
    }

    public ErrorMessage createTimeoutError() {
        return ErrorMessage.createTimeout("Error executing '" + execution + "': " + " Chain timed out.");
    }

}
