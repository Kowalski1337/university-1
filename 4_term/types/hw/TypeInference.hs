module TypeInference where

import Expression
import Data.Maybe (mapMaybe)
import Control.Monad (foldM)
import qualified Data.Set as Set
import qualified Data.Map.Strict as Map
import Data.List (nub, sort, find)

import Control.Monad.State.Lazy

data Equation = Type :=: Type deriving (Eq, Ord)
type System = [Equation]
infix 8 :=:

instance Show Equation where
    show (t1 :=: t2) = show t1 ++ " = " ++ show t2

getBaseTypes :: Type -> Set.Set TypeName
getBaseTypes (BaseType name) = Set.singleton name
getBaseTypes (t1 :>: t2) = Set.union (getBaseTypes t1) (getBaseTypes t2)

typeSubstitute :: Type -> TypeName -> Type -> Type
typeSubstitute t@(BaseType name) subName subType
    | name == subName = subType
    | otherwise       = t
typeSubstitute (t1 :>: t2) subName subType =
        typeSubstitute t1 subName subType :>: typeSubstitute t2 subName subType

resolveStep :: System -> Maybe (System, Bool)
resolveStep system = (\(system4, flag4) -> (system4, flag1 || flag2 || flag3 || flag4))
        `fmap` maybeStep4
  where
    stepReverse system = (newSystem, or flags)
      where
        (newSystem, flags) = unzip $ map reverseEquation system
        reverseEquation (t@(_ :>: _) :=: x@(BaseType _)) = (x :=: t, True)
        reverseEquation e                                = (e,      False)

    stepTautology system = (newSystem, length newSystem /= length system)
      where
        newSystem = mapMaybe dropTaut system
        dropTaut e@(a :=: b)
            | a == b    = Nothing
            | otherwise = Just e

    stepUnfold system = (newSystem, length newSystem /= length system)
      where
        newSystem = concatMap doUnfold system
        doUnfold ((t1 :>: t2) :=: (t3 :>: t4)) = [t1 :=: t3, t2 :=: t4]
        doUnfold e                             = [e]

    stepSubstitute system = case maybeEq of
            Nothing                               -> Just (system, False)
            Just eq@(BaseType name :=: otherType) ->
                    if Set.member name (getBaseTypes otherType)
                        then Nothing
                        else Just (nub $ sort $ map (substWith eq) system, True)
      where
        maybeEq = find check system
        check eq1@(BaseType name :=: _) = any checkOther system
          where
            checkOther eq2@(type1 :=: type2) = eq1 /= eq2 &&
                    any (Set.member name) [getBaseTypes type1, getBaseTypes type2]
        check _ = False
        substWith eq1@(BaseType name :=: otherType) eq2@(type1 :=: type2) =
                if eq1 /= eq2
                    then typeSubstitute type1 name otherType
                            :=: typeSubstitute type2 name otherType
                    else eq1

    (system1, flag1) = stepReverse system
    (system2, flag2) = stepTautology system1
    (system3, flag3) = stepUnfold system2
    maybeStep4       = stepSubstitute system3

resolveSystem :: System -> Maybe System
resolveSystem system = resolveStep system >>=
        (\(system', flag) -> if flag then resolveSystem system' else Just system')