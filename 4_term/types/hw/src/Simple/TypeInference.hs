module Simple.TypeInference where

import           Control.Monad.State.Lazy
import           Data.List                (find, nub, sort)
import qualified Data.Map.Strict          as Map
import           Data.Maybe               (mapMaybe)
import qualified Data.Set                 as Set

import           Simple.Expression
import           Simple.Reduction

data Equation = Type :=: Type deriving (Eq, Ord)
type System = [Equation]
infix 8 :=:

instance Show Equation where
    show (t1 :=: t2) = show t1 ++ " = " ++ show t2

getBaseTypes :: Type -> Set.Set TypeName
getBaseTypes (BaseType name) = Set.singleton name
getBaseTypes (t1 :>: t2)     = Set.union (getBaseTypes t1) (getBaseTypes t2)

typeSubstitute :: Type -> TypeName -> Type -> Type
typeSubstitute t@(BaseType name) subName subType
    | name == subName = subType
    | otherwise       = t
typeSubstitute (t1 :>: t2) subName subType =
        typeSubstitute t1 subName subType :>: typeSubstitute t2 subName subType

resolveStep :: System -> Maybe (System, Bool)
resolveStep system' = do
    let (system1, flag1) = stepReverse system'
    let (system2, flag2) = stepTautology system1
    (system3, flag3) <- stepUnfold system2
    (system4, flag4) <- stepSubstitute system3
    pure (system4, flag1 || flag2 || flag3 || flag4)
  where
    stepReverse system = (newSystem, or flags)
      where
        (newSystem, flags) = unzip $ map reverseEquation system
        reverseEquation e@(t :=: x@(BaseType _)) = case t of
            BaseType _ -> (e,       False)
            _          -> (x :=: t, True)
        reverseEquation e = (e, False)

    stepTautology system = (newSystem, length newSystem /= length system)
      where
        newSystem = mapMaybe dropTaut system
        dropTaut e@(a :=: b)
            | a == b    = Nothing
            | otherwise = Just e

    stepUnfold system = fmap makeNewSystem maybeSystem
      where
        doUnfold ((t1 :>: t2) :=: (t3 :>: t4)) = Just [t1 :=: t3, t2 :=: t4]
        doUnfold e@(BaseType _ :=: BaseType _) = Just [e]
        doUnfold _                             = Nothing
        maybeSystem = mapM doUnfold system
        makeNewSystem newSystem' = (newSystem, length newSystem /= length system)
          where
            newSystem = concat newSystem'

    stepSubstitute system = case maybeEq of
            Just eq@(BaseType name :=: otherType) ->
                    if Set.member name (getBaseTypes otherType)
                        then Nothing
                        else Just (nub $ sort $ map (substWith eq) system, True)
            _ -> Just (system, False)
      where
        maybeEq = find check system
        check eq1@(BaseType name :=: _) = any checkOther system
          where
            checkOther eq2@(type1 :=: type2) = (eq1 /= eq2 || Set.member name (getBaseTypes type2)) &&
                    any (Set.member name) [getBaseTypes type1, getBaseTypes type2]
        check _ = False
        substWith eq1@(BaseType name :=: otherType) eq2@(type1 :=: type2) =
                if eq1 /= eq2
                    then typeSubstitute type1 name otherType
                            :=: typeSubstitute type2 name otherType
                    else eq1
        substWith _ eq = eq

resolveSystem :: System -> Maybe System
resolveSystem system = resolveStep system >>=
        (\(system', flag) -> if flag then resolveSystem system' else Just system')

applySystem :: System -> Type -> Type
applySystem system = apply
  where
    systemMap = Map.fromList $ map (\(a :=: b) -> (a, b)) system
    apply t@(BaseType _) = Map.findWithDefault t t systemMap
    apply (t1 :>: t2)    = apply t1 :>: apply t2

makeSystem :: Expression -> (System, Type)
makeSystem expr = (rawSystem, exprType)
  where
    renameAbstractions :: Expression -> State (Int, Map.Map Var Var) Expression
    renameAbstractions (V var) = do
        (_, varMap) <- get
        pure $ V $ Map.findWithDefault var var varMap
    renameAbstractions (expr1 :$: expr2) = do
        newExpr1 <- renameAbstractions expr1
        newExpr2 <- renameAbstractions expr2
        pure $ newExpr1 :$: newExpr2
    renameAbstractions (L var expr') = do
        (n, varMap) <- get
        let newVar = show n
        put (n + 1, Map.insert var newVar varMap)
        newExpr <- renameAbstractions expr'
        modify (\(n', _) -> (n', varMap))
        pure $ L newVar newExpr

    makeSystem' :: Expression -> State (Int, System) Type
    makeSystem' (V var) = pure $ BaseType $ "t" ++ var
    makeSystem' (expr1 :$: expr2) = do
        type1 <- makeSystem' expr1
        type2 <- makeSystem' expr2
        (n, system) <- get
        let typeName = "e" ++ show n
        let newType = BaseType typeName
        put (n + 1, type1 :=: type2 :>: newType : system)
        pure newType
    makeSystem' (L var expr') = do
        exprType' <- makeSystem' expr'
        pure $ (BaseType $ "t" ++ var) :>: exprType'

    (exprType, (_, rawSystem)) = runState (makeSystem' $ evalState (renameAbstractions expr) (0, Map.empty)) (0, [])

inferType :: Expression -> Maybe (Type, [(Var, Type)])
inferType expr = do
    system <- maybeSystem
    pure $ (applySystem system exprType, makeContext system)
  where
    (rawSystem, exprType) = makeSystem expr
    maybeSystem = resolveSystem rawSystem
    freeVars = getFreeVars expr
    makeContext system = map (\var -> (var, applySystem system $ BaseType $ "t" ++ var)) $ Set.toList freeVars
