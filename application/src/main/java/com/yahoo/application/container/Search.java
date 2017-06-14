// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.application.container;

import com.google.common.annotations.Beta;
import com.google.common.util.concurrent.ListenableFuture;
import com.yahoo.component.ComponentSpecification;
import com.yahoo.component.chain.Chain;
import com.yahoo.processing.execution.chain.ChainRegistry;
import com.yahoo.processing.rendering.Renderer;
import com.yahoo.search.Query;
import com.yahoo.search.Result;
import com.yahoo.search.Searcher;
import com.yahoo.search.handler.HttpSearchResponse;
import com.yahoo.search.handler.SearchHandler;
import com.yahoo.search.searchchain.SearchChainRegistry;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

/**
 * @author <a href="mailto:einarmr@yahoo-inc.com">Einar M R Rosenvinge</a>
 * @author gjoranv
*/
@Beta
public final class Search extends ProcessingBase<Query, Result, Searcher> {

    private final SearchHandler handler;

    Search(SearchHandler handler) {
        this.handler = handler;
    }

    @Override
    public ChainRegistry<Searcher> getChains() {
        return asChainRegistry(handler.getSearchChainRegistry());
    }

    @Override
    protected Result doProcess(Chain<Searcher> chain, Query request) {
        return handler.searchAndFill(request, chain, handler.getSearchChainRegistry());
    }

    @Override
    @SuppressWarnings("deprecation")
    protected ListenableFuture<Boolean> doProcessAndRender(ComponentSpecification chainSpec,
                                                           Query request,
                                                           Renderer<Result> renderer,
                                                           ByteArrayOutputStream stream) throws IOException {
        Result result = process(chainSpec, request);
        result.getTemplating().setRenderer(renderer);
        return HttpSearchResponse.waitableRender(result, result.getQuery(), renderer, stream);
    }

    @Override
    @SuppressWarnings("deprecation")
    protected Renderer<Result> doGetRenderer(ComponentSpecification spec) {
        return handler.getRendererCopy(spec);
    }

    // TODO: move to SearchHandler.getChainRegistry and deprecate SH.getSCReg?
    private ChainRegistry<Searcher> asChainRegistry(SearchChainRegistry legacyRegistry) {
        ChainRegistry<Searcher> chains = new ChainRegistry<>();
        for (Chain<Searcher> chain : handler.getSearchChainRegistry().allComponents())
            chains.register(chain.getId(), chain);
        chains.freeze();
        return chains;
    }

}
